#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <error.h>
#include <pthread.h>
#include <json-c/arraylist.h>
#include <json-c/json.h>
#include "Func.h"
#include "stateList.h"
#include <sqlite3.h>
#include "Sqlite3Db.h"
#include "GrpMsgHash.h"
#include "privateMsgHash.h"
#include "threadPool.h"

/***********全局变量**************/
int sockfd = 0;
threadPool_t Server_P;
pthread_mutex_t stateMutx;
pthread_mutex_t Db_Mutx;
pthread_mutex_t Hash_Mutx;
pthread_mutex_t GRP_Mutx;
stateList* List = NULL;    
sqlite3 * Data_db;
MsgHash* Hash = NULL;
GpHash* Gp_Hash = NULL;
/*********************************/

void* threadHandle(void* arg)
{   
    /* 获取acceptfd文件描述符 */
    int acceptfd = *(int*)arg;

    //接收缓存区
    char recvBuf[COMMUNICATION_SIZE];
    //发送缓存区
    char sendBuf[COMMUNICATION_SIZE];
    /* sql语句字符串数组 */
    char sqlBuf[BUFFER_SIZE];
    /* 用于存放账号密码 */
    char user_Account[ACCOUNT_SIZE] = {0};


    /* 线程的工作 */
    while(1)
    {   
        /* 这一块要不要考虑放while(1)外面去 */
        char** result = NULL;
        char* errmsg;
        int rows = 0;
        int columns = 0;
        
        /* 读客户端信息，客户端和服务端读写缓存区统一为 512 */
        memset(recvBuf, 0, sizeof(recvBuf));
        memset(sendBuf, 0, sizeof(sendBuf));
        memset(sqlBuf, 0, sizeof(sqlBuf));

        int readBytes = read(acceptfd, recvBuf, sizeof(recvBuf));
        if(readBytes == 0)//对于while中重复等待的read，这一步是不可少的，否则，将会继续往下执行，然后程序很明显就会报错了
        {
            printf("[FD:%d] is disconnect......\n", acceptfd);
            close(acceptfd);
            break;
        }
        
        /* 暂用于测试 */
        printf("%s\n", recvBuf);

        /* 分析客户端动作 */
        struct json_object* readObj = json_tokener_parse(recvBuf);//每次parse生成的json对象记得释放！！！
        
        /* 解析客户端的action */
        struct json_object* actionObj = json_object_object_get(readObj, "action");
        
        int action = json_object_get_int(actionObj);

        switch (action)
        {
            case DUPLICATE_CHECK://注册账号查重
                {
                    struct json_object* registerAccountObj = json_object_object_get(readObj, "账号");
                    const char* registerAccount = json_object_get_string(registerAccountObj);
                    
                    /* 通过sprintf生成数据库查询语句 */
                    sprintf(sqlBuf,"SELECT * FROM USER_DATA WHERE ID = '%s'", registerAccount);
                    
                    //printf("%s\n", sqlBuf);

                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    pthread_mutex_unlock(&Db_Mutx);
                    if(rows > 0)//说明有查询结果，该账号已存在
                    {
                        
                        strncpy(sendBuf, "UnAvailable", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    else//rows等于0，说明该账号在用户信息表中不存在
                    {
                        
                        strncpy(sendBuf, "Available", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    break;
                }
                

            case REGISTER ://注册
                {
                    /* 解析出用户信息json对象 */
                    struct json_object* userDataObj = json_object_object_get(readObj, "userData");
                    /* 从用户信息json对象中解析出各个信息的json对象 */
                    struct json_object* userNameObj = json_object_object_get(userDataObj, "昵称");
                    struct json_object* userSexObj = json_object_object_get(userDataObj, "性别");
                    struct json_object* userAgeObj = json_object_object_get(userDataObj, "年龄");
                    struct json_object* userAccountObj = json_object_object_get(userDataObj, "账号");
                    struct json_object* userPasswordObj = json_object_object_get(userDataObj, "密码");
                    /* 获取各个json对象的C字符串 */
                    const char *name = json_object_get_string(userNameObj);
                    const char *Sex = json_object_get_string(userSexObj);
                    int Age = json_object_get_int(userAgeObj);
                    const char *account = json_object_get_string(userAccountObj);
                    const char *password = json_object_get_string(userPasswordObj);
                    

                    /* 生成sqlite3新增语句 */
                    sprintf(sqlBuf, "INSERT INTO USER_DATA (ID, NAME, AGE, SEX, PASSWORD) VALUES ('%s', '%s', %d, '%s', '%s')", account, name, Age, Sex, password);
                    printf("%s\n", sqlBuf);

                    /* 加锁 */
                    pthread_mutex_lock(&Db_Mutx);
                    int ret = sqlite3_exec(Data_db, sqlBuf, NULL, NULL, NULL);
                    pthread_mutex_unlock(&Db_Mutx);

                    if (ret != SQLITE_OK)
                    {
                        printf("sqlite3_exec: %s\n", sqlite3_errmsg(Data_db));
                        exit(1);
                    }
                    /* 回应客户端注册成功 */
                    strncpy(sendBuf, "registerSuccessful", sizeof(sendBuf) - 1);
                    write(acceptfd, sendBuf, sizeof(sendBuf));
                    break;
                }   
                

            case LOG_ON ://登录
                {    /*  
                        登录，先匹配账号密码，错回复checkError,账号密码正确然后判断是否已经在线，
                        在线就回复online。不在线登陆成功，就回复用户信息
                        1.根据在客户端传来的账号在数据库查修对应的密码，如果查询结果为0，表示账号不存在，回复checkError
                        2.如果查询结果不为0，表示账号存在，然后匹配查询到的密码和客户端的密码，如果密码错误，回复checkError
                        3.如果密码正确，查看是否在线，如果在线，回复online。
                        4.如果不在线，回复用户的信息。
                    */
                        struct json_object* AccountObj = json_object_object_get(readObj, "账号");
                        struct json_object* PasswordObj = json_object_object_get(readObj, "密码");

                        /* 获取客户端传来的账号密码 */
                        const char* userAccount = json_object_get_string(AccountObj);
                        const char* userPassWord = json_object_get_string(PasswordObj);

                        strncpy(user_Account, userAccount, sizeof(user_Account) - 1);

                        /* 生成sqlite3查询语句 */
                        sprintf(sqlBuf,"SELECT PASSWORD FROM USER_DATA WHERE ID = '%s'", userAccount);

                        /* 上锁 */
                        pthread_mutex_lock(&Db_Mutx);
                        /* rows的内存结尾记得释放 */
                        sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                        pthread_mutex_unlock(&Db_Mutx);
                        if(rows > 0)//说明有查询结果，该账号存在
                        {
                            /* 匹配密码,result[0]为属性，result[1]为密码 */
                            if(strncmp(result[1], userPassWord, strlen(userPassWord)) == 0)
                            {
                                /* 账号密码正确, 查询是否在线 */
                                /* 查询账号是否在线,比对账号字符串 */
                                pthread_mutex_lock(&stateMutx);
                                int rc = stateListSearch(List, userAccount);
                                pthread_mutex_unlock(&stateMutx);
                                
                                if(rc == SEARCH_SUCCESS)
                                {
                                    /* 在线，传回onLine */
                                    
                                    strncpy(sendBuf, "onLine", sizeof(sendBuf) - 1);
                                    write(acceptfd, sendBuf, sizeof(sendBuf));
                                }
                                else
                                {
                                    /* 不在线，可以登录，查询数据库所有用户信息，传回用户信息， */
                                    /* 将账号加入在线用户列表 */
                                    pthread_mutex_lock(&stateMutx);
                                    stateListInsert(List, userAccount, acceptfd);
                                    pthread_mutex_unlock(&stateMutx);
                                    /* 查询用户信息并传给客户端，考虑用json */
                                    sprintf(sqlBuf,"SELECT * FROM USER_DATA WHERE ID = '%s'", userAccount);
                                    char **tmpResult = NULL;

                                    /* 上锁 */
                                    pthread_mutex_lock(&Db_Mutx);
                                    /* rows的内存结尾记得释放 */
                                    sqlite3_get_table(Data_db, sqlBuf, &tmpResult, &rows, &columns, &errmsg);           
                                    pthread_mutex_unlock(&Db_Mutx);

                                    struct json_object * userDataBack = json_object_new_object();
                                    json_object_object_add(userDataBack, "ID", json_object_new_string(tmpResult[5]));
                                    json_object_object_add(userDataBack, "NAME", json_object_new_string(tmpResult[6]));
                                    json_object_object_add(userDataBack, "AGE", json_object_new_int(strtol(tmpResult[7], NULL, 10)));//字符串转整形
                                    json_object_object_add(userDataBack, "SEX", json_object_new_string(tmpResult[8]));
                                    json_object_object_add(userDataBack, "PASSWORD", json_object_new_string(tmpResult[9]));
                                    const char * backStr = json_object_to_json_string(userDataBack);

                                    strncpy(sendBuf, backStr, sizeof(sendBuf) - 1);
                                    write(acceptfd, sendBuf, sizeof(sendBuf));

                                    sqlite3_free_table(tmpResult);
                                    json_object_put(userDataBack);
                                }
                                
                            }
                            else
                            {
                                /* 密码错误 */
                                strncpy(sendBuf, "checkError", sizeof(sendBuf) - 1);
                                write(acceptfd, sendBuf, sizeof(sendBuf));
                            }
                        }
                        else//rows等于0，说明该账号在用户信息表中不存在
                        {
                            /* 账号密码错误 */
                            strncpy(sendBuf, "checkError", sizeof(sendBuf) - 1);
                            write(acceptfd, sendBuf, sizeof(sendBuf));
                        }
                        
                    break;
                }

            case ADD_FRIENDS ://添加好友
                {
                    /* 先查询账号是否正确 */
                    /* 再查看是否已经是好友 */
                    /* 当以上条件都满足时,添加数据库数据 */
                    struct json_object* inviterObj = json_object_object_get(readObj, "INVITER");
                    struct json_object* inviteeObj = json_object_object_get(readObj, "INVITEE");

                    /* 获取客户端传来的数据 */
                    const char* inviter = json_object_get_string(inviterObj);//我
                    const char* invitee = json_object_get_string(inviteeObj);//我要加的人
                    printf("%s\n", inviter);
                    printf("%s\n", invitee);
                    /* 先查询数据库用户信息表中是否存在该账号 */
                    sprintf(sqlBuf,"SELECT * FROM USER_DATA WHERE ID = '%s'", invitee);

                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    pthread_mutex_unlock(&Db_Mutx);
                    
                    if(rows > 0)//说明有查询结果，该账号存在
                    {
                        /* 在好友表中查询是否已经是好友 */
                        sprintf(sqlBuf,"SELECT * FROM FRIEND_DATA WHERE (INVITER = '%s' AND INVITEE = '%s' AND DEAL = '好友') OR (INVITER = '%s' AND INVITEE = '%s' AND DEAL = '好友') OR (INVITER = '%s' AND INVITEE = '%s' AND DEAL = '同意') OR (INVITER = '%s' AND INVITEE = '%s' AND DEAL = '同意')", inviter, invitee, invitee, inviter, inviter, invitee, invitee, inviter);
                        
                        /* 分开分配，分开释放，防止内存泄漏 */
                        char** tmpResult = NULL;

                        /* 执行sqlite3查询语句 */
                        pthread_mutex_lock(&Db_Mutx);
                        sqlite3_get_table(Data_db, sqlBuf, &tmpResult, &rows, &columns, &errmsg);
                        pthread_mutex_unlock(&Db_Mutx);
                        if(rows == 0)/* 这里的rows是好友表中的查询结果的行数 */
                        {   
                            
                            /* 好友表中不存在这两人的好友关系，可以发送好友邀请 */
                            sprintf(sqlBuf,"INSERT INTO FRIEND_DATA (INVITER,INVITEE,DEAL) VALUES ('%s','%s','还在考虑')", inviter, invitee);

                            pthread_mutex_lock(&Db_Mutx);
                            int ret = sqlite3_exec(Data_db, sqlBuf, NULL, NULL, NULL);
                            pthread_mutex_unlock(&Db_Mutx);

                            strncpy(sendBuf, "InviteSuccess", sizeof(sendBuf) - 1);
                            write(acceptfd, sendBuf, sizeof(sendBuf));
                        }
                        else
                        {
                            /* 已经是好友 */
                            strncpy(sendBuf, "IsFriend", sizeof(sendBuf) - 1);
                            write(acceptfd, sendBuf, sizeof(sendBuf));
                            
                        }

                        /* 释放 */
                        sqlite3_free_table(tmpResult);
                    }
                    else//rows等于0，说明该账号在用户信息表中不存在
                    {
                        strncpy(sendBuf, "NotExists", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    break;
                }

            case VIEW_MY_INVITE ://查看我发出的好友邀请结果
                {   
                    sprintf(sqlBuf, "SELECT * FROM FRIEND_DATA WHERE INVITER = '%s' AND DEAL != '好友';", user_Account);
                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    pthread_mutex_unlock(&Db_Mutx);

                    if(rows != 0)
                    {
                        int idx = 0;
                        int jdx = 4;//012为列名，4为第一个invitee,5为第一个DEAL,7为第二个invitee......
                        for(idx = 1; idx <= rows; idx++)
                        {
                            /* 第一个%s为接收方的名字，第二个%s为处理情况 */
                            sprintf(sendBuf, "%d.[%s]%s您的好友邀请", idx, result[jdx], result[jdx + 1]);
                            write(acceptfd, sendBuf, sizeof(sendBuf));

                            char **tmpResult = NULL;
                            if(strncmp(result[jdx + 1], "同意", strlen("同意")) == 0)
                            {   
                                /* 将DEAL更新为好友 */
                                sprintf(sqlBuf, "UPDATE FRIEND_DATA SET DEAL = '好友' WHERE INVITER = '%s' AND INVITEE = '%s';", result[jdx - 1], result[jdx]);
                                
                                /* 执行sql语句 */
                                pthread_mutex_lock(&Db_Mutx);
                                sqlite3_get_table(Data_db, sqlBuf, &tmpResult, NULL, NULL, &errmsg);
                                pthread_mutex_unlock(&Db_Mutx);
                            }
                            else if(strncmp(result[jdx + 1], "拒绝", strlen("拒绝")) == 0)
                            {
                                /* 删除对应的DEAL数据 */
                                sprintf(sqlBuf, "DELETE FROM FRIEND_DATA WHERE INVITER = '%s' AND INVITEE = '%s' AND DEAL = '拒绝';", result[jdx - 1], result[jdx]);
                                
                                /* 执行sql语句 */
                                pthread_mutex_lock(&Db_Mutx);
                                sqlite3_get_table(Data_db, sqlBuf, &tmpResult, NULL, NULL, &errmsg);
                                pthread_mutex_unlock(&Db_Mutx);
                            }

                            jdx += 3; //jdx移到下一个对应的位置

                            if(tmpResult != NULL)
                            {
                                sqlite3_free_table(tmpResult);
                            }
                        }
                        /* 结束了就告诉客户端查询结束 */
                        memset(sendBuf, 0, sizeof(sendBuf));
                        strncpy(sendBuf, "FINISH", sizeof(sendBuf));
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    else
                    {   
                        /* 没有发给他人的待处理好友邀请 */
                        strncpy(sqlBuf, "NotInvite", sizeof(sqlBuf) - 1);
                        write(acceptfd, sqlBuf, sizeof(sqlBuf));
                    }
                    break;
                }

            case VIEW_OTHER_INVITE ://查看和处理请求加我为好友的验证消息
                {
                    /* SELECT * FROM FRIEND_DATA WHERE INVITEE = 'MyAccount' AND DEAL = '还在考虑' */
                    /* 程序能执行到这个功能，账号密码已经保存在服务端中 */
                    /* sqlite3查询 */

                    sprintf(sqlBuf, "SELECT INVITER FROM FRIEND_DATA WHERE INVITEE = '%s' AND DEAL = '还在考虑'", user_Account);
                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    pthread_mutex_unlock(&Db_Mutx);

                    /* 查询结果仅为发起邀请的账号 */
                    if(rows == 0)
                    {
                        /* 说明没有验证消息 */
                        strncpy(sendBuf, "NotVerifyMessage", sizeof(sendBuf));
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    else
                    {
                        /* 说明有验证消息，发给客户端 */
                        int idx = 0;//0是列名
                        for(idx = 1; idx <= rows; idx++)//这里sqlite3仅查询一个结果
                        {   
                            sprintf(sendBuf, "%d.[%s]向您发起了好友邀请", idx, result[idx]);
                            write(acceptfd, sendBuf, sizeof(sendBuf));
                        }

                        /* for循环结束后，已经将查询到的result发送完毕，告诉客户端执行下一步操作 */
                        memset(sendBuf, 0, sizeof(sendBuf));
                        strncpy(sendBuf, "FINISH", sizeof(sendBuf));    
                        write(acceptfd, sendBuf, sizeof(sendBuf));

                        /* 读取客户端下一步操作指令 */
                        /*  */
                        memset(recvBuf, 0, sizeof(recvBuf));
                        read(acceptfd, recvBuf, sizeof(recvBuf));
                        
                        /* 解析客户端需求 */
                        struct json_object* choicesObj = json_tokener_parse(recvBuf);
                        struct json_object* NumsChoiceObj = json_object_object_get(choicesObj, "NumsChoice");
                        struct json_object* choiceObj = json_object_object_get(choicesObj, "choice"); 
                        
                        /* 获取选择编号和选择 */
                        int NumsChoice = json_object_get_int(NumsChoiceObj);
                        int choice = json_object_get_int(choiceObj);
                        
                        char* Deal = NULL;
                        if(choice == 0)
                        {
                            Deal = "拒绝";
                        }
                        else if(choice == 1)
                        {
                            Deal = "同意";
                        }
                        

                        char **tmpResult = NULL;
                        /* 执行sql语句 */
                        sprintf(sqlBuf, "UPDATE FRIEND_DATA SET DEAL = '%s' WHERE INVITEE = '%s' AND INVITER = '%s';", Deal, user_Account, result[NumsChoice]);
                        pthread_mutex_lock(&Db_Mutx);
                        sqlite3_get_table(Data_db, sqlBuf, &tmpResult, NULL, NULL, &errmsg);
                        pthread_mutex_unlock(&Db_Mutx);

                        sqlite3_free_table(tmpResult);
                        json_object_put(choicesObj);
                    }

                    break;
                }
            case PRIVATE_CHAT : //查看好友列表和私聊
                {
                    /* 先查询好友，将存在的好友发过去 */
                    //SELECT INVITER FROM FRIEND_DATA WHERE INVITEE = '%s' AND DEAL = '好友' UNION SELECT INVITEE FROM FRIEND_DATA WHERE INVITER = '%s' AND DEAL = '好友'
                    sprintf(sqlBuf, "SELECT INVITER FROM FRIEND_DATA WHERE INVITEE = '%s' AND DEAL = '好友' UNION SELECT INVITEE FROM FRIEND_DATA WHERE INVITER = '%s' AND DEAL = '好友';", user_Account, user_Account);
                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    pthread_mutex_unlock(&Db_Mutx);
                    
                    /* 先判断是否有好友 */
                    if(rows == 0)
                    {
                        /* 告诉客户端没有好友 */
                        strncpy(sendBuf, "NotFriends", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    else
                    {
                        /* 用于查询对应的NAME */
                        char** tmpResult = NULL;
                        int tmpRows = 0;
                        int tmpColumns = 0;
                        char* tmpErrmsg = NULL;
                        char tmpSql[BUFFER_SIZE] = {0};
                        printf("rows:%d\n", rows);
                        int idx = 0;
                        for(idx = 1; idx <= rows; idx++)
                        {   
                            /* 上锁查询对应的在线状态 */
                            pthread_mutex_lock(&stateMutx);
                            int ret = stateListSearch(List, result[idx]);
                            pthread_mutex_unlock(&stateMutx);
                            
                            sprintf(tmpSql, "SELECT NAME FROM USER_DATA WHERE ID = '%s'", result[idx]);
                            
                            /* 执行sqlite3查询语句 */
                            pthread_mutex_lock(&Db_Mutx);
                            sqlite3_get_table(Data_db, tmpSql, &tmpResult, &tmpRows, &tmpColumns, &tmpErrmsg);
                            pthread_mutex_unlock(&Db_Mutx);

                            if(ret == SEARCH_SUCCESS)
                            {
                                sprintf(sendBuf, "%d.[%s](在线)", idx, tmpResult[1]);
                            }
                            else
                            {
                                sprintf(sendBuf, "%d.[%s](离线)", idx, tmpResult[1]);
                            }

                            write(acceptfd, sendBuf, sizeof(sendBuf));
                            sqlite3_free_table(tmpResult);
                        }
                        /* 循环结束，告诉客户端可以停止读了 */
                        memset(sendBuf, 0, sizeof(sendBuf));
                        strncpy(sendBuf, "FINISH", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));    

                        /* 等待客户端指令 */
                        /* 选择和谁聊天 */
                        /* 不选择和任何人聊天，退出该case */
                        memset(recvBuf, 0, sizeof(recvBuf));
                        read(acceptfd, recvBuf, sizeof(recvBuf));
                        if(strncmp(recvBuf, "NO_CHAT", strlen("NO_CHAT")) != 0)
                        {
                            /* 说明弱智客户端发来的不是NO_CHAT,弱智客户想聊天，发来的内容为choice好友编号 */
                            struct json_object* chat_choiceObj = json_tokener_parse(recvBuf);
                            struct json_object* NumsObj = json_object_object_get(chat_choiceObj, "chatChoice");
                            /* 获取选择result编号 */
                            int FR_Choice = json_object_get_int(NumsObj);
                            //result[FR_Choice]客户想要发消息的好友的账号
                            /* 开始接收客户端发给好友的消息和搜寻消息队列的消息 */
                            dealPrivateChat(acceptfd, user_Account, result[FR_Choice], Hash, Data_db, &Hash_Mutx);
                            json_object_put(chat_choiceObj);
                        }
                    }

                    break;
                }
            case CREATE_GROUP ://创建建群
                {
                    struct json_object* GroupName = json_object_object_get(readObj, "GroupName");
                    const char* G_Name = json_object_get_string(GroupName);
                    /* 先查询是否已经存在该群名 */
                    sprintf(sqlBuf, "SELECT * FROM GROUP_DATA WHERE GROUP_NAME = '%s'", G_Name);
                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    printf("rows:%d\n", rows);
                    pthread_mutex_unlock(&Db_Mutx);
                    if(rows == 0)
                    {
                        /* 不存在，插入数据库 */
                        char tmpSql[BUFFER_SIZE] = {0};
                        sprintf(tmpSql, "INSERT INTO GROUP_DATA (GROUP_NAME,MEMBER) VALUES ('%s','%s')", G_Name, user_Account);
                        /* 执行插入语句 */
                        pthread_mutex_lock(&Db_Mutx);
                        sqlite3_exec(Data_db, tmpSql, NULL, NULL, NULL);
                        pthread_mutex_unlock(&Db_Mutx);
                        /* 告诉客户端成功 */
                        strncpy(sendBuf, "SUCCESS", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    else
                    {
                        /* 存在，告知客户端重新取名 */
                        strncpy(sendBuf, "FAIL", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    break;
                }
            case ADD_GROUP : //添加群聊
                {
                    struct json_object* GroupName = json_object_object_get(readObj, "GroupName");
                    const char* GRP_Name = json_object_get_string(GroupName);
                    /* 先查询是否已经存在该群名 */
                    sprintf(sqlBuf, "SELECT * FROM GROUP_DATA WHERE GROUP_NAME = '%s'", GRP_Name);
                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    pthread_mutex_unlock(&Db_Mutx);
                    printf("Test:rows:%d\n", rows);
                    if(rows == 0)
                    {
                        /* 该群不存在 */
                        strncpy(sendBuf, "GROUP_NOT_EXISTS", sizeof(sendBuf));
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    else
                    {
                        /* 群存在，判断是否已经是该群成员 */
                        char** tmpResult = NULL;
                        int tmpRows = 0;
                        int tmpColumns = 0;
                        char* tmpErrmsg = NULL;
                        char tmpSql[BUFFER_SIZE] = {0};

                        sprintf(tmpSql, "SELECT * FROM GROUP_DATA WHERE GROUP_NAME = '%s' AND MEMBER = '%s'", GRP_Name, user_Account);
                        /* 执行sqlite3查询语句 */
                        pthread_mutex_lock(&Db_Mutx);
                        sqlite3_get_table(Data_db, tmpSql, &tmpResult, &tmpRows, &tmpColumns, &tmpErrmsg);
                        pthread_mutex_unlock(&Db_Mutx);
                        
                        if(tmpRows == 0)
                        {
                            /* 不是群成员，加入 */
                            sprintf(tmpSql, "INSERT INTO GROUP_DATA (GROUP_NAME,MEMBER) VALUES ('%s','%s')", GRP_Name, user_Account);
                            /* 执行插入语句 */
                            pthread_mutex_lock(&Db_Mutx);
                            sqlite3_exec(Data_db, tmpSql, NULL, NULL, NULL);
                            pthread_mutex_unlock(&Db_Mutx);
                            /* 告诉客户端成功 */
                            memset(sendBuf, 0, sizeof(sendBuf));
                            strncpy(sendBuf, "ADDSUCCESS", sizeof(sendBuf));
                            write(acceptfd, sendBuf, sizeof(sendBuf));
                        }
                        else
                        {  
                            /* 已经是群成员 */
                            memset(sendBuf, 0, sizeof(sendBuf));
                            strncpy(sendBuf, "ISMEMBER", sizeof(sendBuf));
                            write(acceptfd, sendBuf, sizeof(sendBuf));
                        }

                        sqlite3_free_table(tmpResult);
                    }
                    break;
                }
            case GROUP_CHAT ://群聊
                {
                    /* 查询用户所在的群名 */
                    sprintf(sqlBuf, "SELECT GROUP_NAME FROM GROUP_DATA WHERE MEMBER = '%s'", user_Account);
                    /* 执行sqlite3查询语句 */
                    pthread_mutex_lock(&Db_Mutx);
                    sqlite3_get_table(Data_db, sqlBuf, &result, &rows, &columns, &errmsg);
                    pthread_mutex_unlock(&Db_Mutx);

                    if(rows == 0)
                    {
                        strncpy(sendBuf, "NotGroup", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));
                    }
                    else
                    {
                        int idx = 0;
                        for(idx = 1; idx <= rows; idx++)
                        {
                            sprintf(sendBuf, "%d.[%s]", idx, result[idx]);
                            write(acceptfd, sendBuf, sizeof(sendBuf));
                        }

                        /* 告诉客户端停止读 */
                        memset(sendBuf, 0, sizeof(sendBuf));
                        strncpy(sendBuf, "FINISH!@#$%^&*", sizeof(sendBuf) - 1);
                        write(acceptfd, sendBuf, sizeof(sendBuf));

                        /* 等待客户端回应聊天或者不选择聊天 */
                        memset(recvBuf, 0, sizeof(recvBuf));
                        read(acceptfd, recvBuf, sizeof(recvBuf));

                        if(strncmp(recvBuf, "NO_CHAT", strlen("NO_CHAT")) != 0)
                        {
                            /* 客户发来的不是NO_CHAT,聊天 */
                            /* 说明弱智客户端发来的不是NO_CHAT,弱智客户想聊天，发来的内容为choice好友编号 */
                            struct json_object* GRP_choiceObj = json_tokener_parse(recvBuf);
                            struct json_object* GRP_NumsObj = json_object_object_get(GRP_choiceObj, "GroupChoice");
                            /* 获取选择result编号 */
                            int GRP_Choice = json_object_get_int(GRP_NumsObj);
                            //result[GRP_Choice]客户想要发消息的群聊
                            /* 开始接收客户端发给好友的消息和搜寻消息队列的消息 */
                            // dealPrivateChat(acceptfd, user_Account, result[FR_Choice], Hash, Data_db, &Hash_Mutx);
                            /* 服务端:处理客户端群聊 */
                            printf("step1\n");
                            dealGrpChat(acceptfd, user_Account, result[GRP_Choice], Gp_Hash, Data_db, &GRP_Mutx, &Db_Mutx);
                            json_object_put(GRP_choiceObj);
                        }

                    }
                    break;
                }
            case LOG_OUT ://退出登录
                {
                    printf("?????%d\n", action);
                    /* 将该账号从在线用户列表中删除 */
                    pthread_mutex_lock(&stateMutx);
                    stateListAppointValDel(List, user_Account);//登录时的账号存放在userAccount中
                    pthread_mutex_unlock(&stateMutx);
                    printf("[USER:%s] out the chat Room!\n", user_Account);
                    break;
                }

            default:
                break;
        }

        sqlite3_free_table(result);
        json_object_put(readObj);
    }

    close(acceptfd);
}

/***************************************/
/* 服务端退出信号处理函数Ctrl + C */
void SIG_Handler()
{
    printf("SEVER_CLOSING.....\n");
    stateListDestroy(List);
    printf("STATE_LIST_DESTROYED!\n");
    threadPoolDestroy(&Server_P);
    printf("THREAD_POOL_DESTROYED!\n");
    pthread_mutex_destroy(&stateMutx);
    pthread_mutex_destroy(&Db_Mutx);
    pthread_mutex_destroy(&Hash_Mutx);
    printf("ALL_MUTEX_DESTROYED!\n");
    close(sockfd);
    printf("SOCKFD_CLOSED!\n");
    exit(0);
}
/***************************************/

int main()
{
    signal(SIGINT, SIG_Handler);
    pthread_t tid;
    int ret = 0;

/*********************************************/
    /* 初始化在线用户列表 */
    stateListInit(&List);
    /* 私聊hash初始化 */
    HashInit(&Hash, HASH_KEY_SIZE);
    /* 群聊哈希初始化 */
    GpHashInit(&Gp_Hash, HASH_KEY_SIZE);
    /* 线程池初始化 */
    threadPoolInit(&Server_P,MIN_THREAD_NUMS, MAX_THREAD_NUMS, TASK_CAPACITY);
    /* 用户在线状态锁初始化 */
    pthread_mutex_init(&stateMutx, NULL);
    /* 数据库锁初始化 */
    pthread_mutex_init(&Db_Mutx, NULL);
    /* 私聊哈希锁初始化 */
    pthread_mutex_init(&Hash_Mutx, NULL);
    /* 群聊哈希锁初始化 */
    pthread_mutex_init(&GRP_Mutx, NULL);
/*********************************************/

    /* 连接数据库 */
    ret = sqlite3_open("Data.db", &Data_db);
    if(ret != SQLITE_OK)
    {
        printf("sqlite3_open: %s\n", sqlite3_errmsg(Data_db));
        exit(1);
    }

/***************sqlite3_table******************/
    /* 创建用户信息表 */
    sqlite3UserTableCreate(Data_db);
    /* 创建群聊成员信息表 */
    sqlite3GroupTableCreate(Data_db);
    /* 创建好友关系表 */
    sqlite3FriendshipTableCreate(Data_db);
/**********************************************/

    /* 创建服务端套接字 */
    SrSocket(&sockfd, SERVER_PORT);

/********************创建与客户端的通信*********************************************************************************/
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(struct sockaddr_in));
    while(1)
    {
        /* 接收客户端的连接 */
        socklen_t clientAddressLen = sizeof(clientAddress);
        int acceptfd = accept(sockfd, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (acceptfd == -1)
        {
            perror("accpet error");
            exit(-1);
        }
        threadPoolAddTask(&Server_P, threadHandle, (void*)&acceptfd);
    }
/**********************************************************************************************************************/
}