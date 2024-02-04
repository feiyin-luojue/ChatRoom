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
#include <json-c/json_object.h>
#include <json-c/json.h>
#include "Func.h"
#include "stateList.h"
#include <sqlite3.h>
#include "Sqlite3Db.h"
#include "privateMsgHash.h"
#include "GrpMsgHash.h"
#include <sys/fcntl.h>



/* 静态 */
/* 发送和接收消息，读写分离 */
static int privateMsgChat(int sockfd);

/* 群聊读写分离 */
static int GrpMsgChat(int sockfd);

/* 服务端套接字创建函数，传出参数获取套接字描述符，第二个参数为端口号 */
int SrSocket(int * sockfdGet, int serverPort)
{

    /* 创建socket套接字 */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket error");
        exit(-1);
    }

    /* 设置端口复用 */
    int enableOpt = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&enableOpt, sizeof(enableOpt));
    if (ret == -1)
    {
        perror("setsockopt error");
        exit(-1);
    }


    struct sockaddr_in localAddress;

    /* 清除脏数据 */
    memset(&localAddress, 0, sizeof(localAddress));

    /* 地址族 */
    localAddress.sin_family = AF_INET;
    /* 端口需要转成大端 */
    localAddress.sin_port = htons(serverPort);
    /* ip地址需要转成大端 */

    /* Address to accept any incoming messages.  */
    /* INADDR_ANY = 0x00000000 */
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY); 

    
    int localAddressLen = sizeof(localAddress);
    ret = bind(sockfd, (struct sockaddr *)&localAddress, localAddressLen);
    if (ret == -1)
    {
        perror("bind error");
        exit(-1);
    }

    /* 监听 */
    ret = listen(sockfd, MAX_LISTEN);
    if (ret == -1)
    {
        perror("listen error");
        exit(-1);
    }

    *sockfdGet = sockfd;

    return 0;
}

/* 客户端创建套接字函数，传出参数获取套接字描述符，第二个参数为服务端端口号，第三个参数为服务端IP */
int clnSocket(int * sockfdGet, int server_port, const char* server_ip)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket error");
        exit(-1);
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    /* 端口 */
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(server_port);
    /* IP地址 */
    int ret = inet_pton(AF_INET, server_ip, (void *)&(serverAddress.sin_addr.s_addr));
    if (ret != 1)
    {
        perror("inet_pton error");
        exit(-1);
    }
    
    /* ip地址 */
    ret = connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (ret == -1)
    {
        perror("connect error");
        exit(-1);
    }

    *sockfdGet = sockfd;

    return 0;
}

/* 新建一个用户信息 */
int Register(int sockfd, char *buf)
{
    int ret = 0;

    /* 年龄 */
    int age = 0;
    /* 昵称 */
    char name[BUFFER_SIZE];
    memset(name, 0, sizeof(char) * BUFFER_SIZE);
    /* 性别 */
    char sex[BUFFER_SIZE];
    memset(sex, 0, sizeof(char) * BUFFER_SIZE);
    /* 账号 */
    char account[BUFFER_SIZE];
    /* 密码 */
    char password[BUFFER_SIZE];
    memset(password, 0, sizeof(char) * BUFFER_SIZE);

    /* 获取合法账号以及访问服务端账号是否存在 */
    /* 向服务端发起确认账号是否已存在 */
    /* 向服务器发送一个包含行动和数据的json格式的字符串 */
    struct json_object *accountObj = json_object_new_object();
    while (1)
    {
        int flag = 0;
        int flag1 = 1;
        /* 获取纯数字的账号 */
        while (flag == 0)
        {
            flag = 1;
            memset(account, 0, sizeof(char) * BUFFER_SIZE);
            system("clear");
            printf("账号(10位整数):");
            scanf("%s", account);
            if (strlen(account) != 10)
            {
                system("clear");
                if (flag1 == 1)
                {
                    printf("请输入10位整数\n");
                }
                else
                {
                    printf("小只因：不识字吗?\n");
                }
                flag1 = 0;
                flag = 0;
                sleep(1);
                while (getchar() != '\n');
            }
            else
            {
                for (int idx = 0; idx < strlen(account); idx++)
                {
                    if (account[idx] < '0' || account[idx] > '9')
                    {
                        printf("无效的输入\n");
                        sleep(1);
                        flag = 0;
                        break;
                    }
                }
            }
        }

        /* 添加行动和数据 */
        json_object_object_add(accountObj, "action", json_object_new_int(DUPLICATE_CHECK)); // 查重
        json_object_object_add(accountObj, "账号", json_object_new_string(account));
        /* 转换成字符串json格式 */
        const char *sendStr = json_object_to_json_string(accountObj);

        /* 发送缓存区 */
        char sendBuf[COMMUNICATION_SIZE];
        memset(sendBuf, 0, sizeof(sendBuf));

        strncpy(sendBuf, sendStr, sizeof(sendBuf) - 1);
        /* 发送信息 */
        write(sockfd, sendBuf, sizeof(sendBuf));

        /* 接收缓存区 */
        char recvBuf[COMMUNICATION_SIZE];
        memset(recvBuf, 0, sizeof(sendBuf));

        /* 等待服务端回应 */
        read(sockfd, recvBuf, sizeof(recvBuf));
        /* 服务器发送available表示该账号可用 */
        if (strncmp(recvBuf, "Available", strlen("Available")) == 0)
        {
            break;
        }
        else
        {
            printf("该账号已存在\n");
            sleep(1);
        }
    }

    system("clear");
    printf("昵称：");
    scanf("%s", name);

    int sexNumsChoice = 0;
    while (1)
    {
        system("clear");
        printf("选择性别(1.男 2.女):");
        scanf("%d", &sexNumsChoice);
        /* 清空缓存区 */
        while (getchar() != '\n');

        if (sexNumsChoice == 1)
        {
            strncpy(sex, "男", sizeof(sex) - 1);
            break;
        }

        if (sexNumsChoice == 2)
        {
            strncpy(sex, "女", sizeof(sex) - 1);
            break;
        }

        printf("小只因：没长眼睛？\n");
        sleep(1);
    }

    char tmp[BUFFER_SIZE];
    /* age为int型，防止输入字符型出错，保证用户输入为int型 */
    while (1)
    {
        memset(tmp, 0, sizeof(char) * BUFFER_SIZE);
        system("clear");
        printf("年龄：");
        scanf("%s", tmp);
        if (sscanf(tmp, "%d", &age) == 1 && age > 0 && age <= 100)
        {
            if (age < 23)
            {
                printf("小只因：你要叫我哥哥\n");
                sleep(1);
            }
            break;
        }
        else
        {
            printf("小只因：电脑前坐的是个人?\n");
            sleep(1);
        }
    }


    int flag = 0;
    int hasCapitalEnglish = 0;
    int hasLowercaseEnglish = 0;
    int hasNumbers = 0;
    int hasSpecialCharacters = 0;
    while (flag == 0)
    {
        while (getchar() != '\n');
        system("clear");
        printf("密码（必须包含大小写英文、数字和特殊字符(！、#、$、&、@):");
        scanf("%s", password);
        for (int idx = 0; idx < strlen(password); idx++)
        {
            if ((password[idx] > 63) && (password[idx] < 91))
            {
                hasCapitalEnglish = 1;
            }
            else if ((password[idx] > 96) && (password[idx] < 123))
            {
                hasLowercaseEnglish = 1;
            }
            else if ((password[idx] > 32) && (password[idx] < 39))
            {
                hasSpecialCharacters = 1;
            }
            else if ((password[idx] >= '0') && (password[idx] <= '9'))
            {
                hasNumbers = 1;
            }
        }
        if (hasCapitalEnglish && hasLowercaseEnglish && hasSpecialCharacters && hasNumbers)
        {
            printf("密码注册成功！\n");
            sleep(1);
            flag = 1;
        }
        else
        {
            hasCapitalEnglish = 0;
            hasLowercaseEnglish = 0;
            hasNumbers = 0;
            hasSpecialCharacters = 0;
            printf("密码不满足注册条件！\n");
            sleep(1);
        }
    }
    

    struct json_object *userData = json_object_new_object();
    
    json_object_object_add(userData, "昵称", json_object_new_string(name));
    json_object_object_add(userData, "性别", json_object_new_string(sex));
    json_object_object_add(userData, "年龄", json_object_new_int(age));
    json_object_object_add(userData, "账号", json_object_new_string(account));
    json_object_object_add(userData, "密码", json_object_new_string(password));


    /* 用户信息json字符串 */
    const char *str = json_object_to_json_string(userData);

    struct json_object *userDataSend = json_object_new_object();
    const char *sendstr = NULL;

    /* 定义register注册行动 */
    json_object_object_add(userDataSend, "action", json_object_new_int(REGISTER));

    /* 将用户信息包装好放入，并标明键为userData */
    json_object_object_add(userDataSend, "userData", userData);
    sendstr = json_object_to_json_string(userDataSend);

    char sendBuf[COMMUNICATION_SIZE];
    memset(sendBuf, 0, sizeof(sendBuf));

    char recvBuf[COMMUNICATION_SIZE];
    memset(recvBuf, 0, sizeof(recvBuf));

    strncpy(sendBuf, sendstr, sizeof(sendBuf) - 1);

    /* 将包装好的行动信息和数据发送 */
    write(sockfd, sendBuf, sizeof(sendBuf));

    read(sockfd, recvBuf, sizeof(recvBuf));
    if (strncmp(recvBuf, "registerSuccessful", strlen("registerSuccessful")) == 0)
    {
        system("clear");
        printf("注册成功！您的用户信息：%s\n", str);
        sleep(1);
    }

    /* 将字符串复制给传进的字符串数组 */
    strncpy(buf, str, (sizeof(char) * COMMUNICATION_SIZE) - 1);

    /* 释放json对象 */
    json_object_put(accountObj);
    json_object_put(userDataSend);

    return ret;
}


/* 登录 */ /* 参数1：套接字文件描述符，参数2：用于接服务器传回的个人信息 */
int logon(int sockfd, userData* MyData)
{
    int ret = 0;
    int true = 0;
    struct json_object* logOnObj = json_object_new_object();

    char account[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];
    
    while(!true)
    {
        /* 获取合法账号以及访问服务端账号是否存在 */
        /* 该账号是否已在登录状态 */
        /* 向服务器发送一个包含行动和数据的json格式的字符串 */
            int flag = 0;
            int flag1 = 1;
            /* 获取纯数字的账号 */
            while (flag == 0)
            {
                flag = 1;
                memset(account, 0, sizeof(char) * BUFFER_SIZE);
                system("clear");
                printf("账号(10位整数):");
                scanf("%s", account);
                if (strlen(account) != 10)
                {
                    system("clear");
                    if (flag1 == 1)
                    {
                        printf("请输入10位整数\n");
                    }
                    else
                    {
                        printf("小只因：不识字吗?\n");
                    }
                    flag1 = 0;
                    flag = 0;
                    sleep(1);
                    while (getchar() != '\n');
                }
                else
                {
                    for (int idx = 0; idx < strlen(account); idx++)
                    {
                        if (account[idx] < '0' || account[idx] > '9')
                        {
                            printf("无效的输入\n");
                            sleep(1);
                            flag = 0;
                            break;
                        }
                    }
                }
                
            }


        int flag2 = 0;
        int hasCapitalEnglish = 0;
        int hasLowercaseEnglish = 0;
        int hasNumbers = 0;
        int hasSpecialCharacters = 0;
        while (flag2 == 0)
        {
            while (getchar() != '\n');
            system("clear");
            printf("密码（必须包含大小写英文、数字和特殊字符(！、#、$、&、@):");
            scanf("%s", password);
            for (int idx = 0; idx < strlen(password); idx++)
            {
                if ((password[idx] > 63) && (password[idx] < 91))
                {
                    hasCapitalEnglish = 1;
                }
                else if ((password[idx] > 96) && (password[idx] < 123))
                {
                    hasLowercaseEnglish = 1;
                }
                else if ((password[idx] > 32) && (password[idx] < 39))
                {
                    hasSpecialCharacters = 1;
                }
                else if ((password[idx] >= '0') && (password[idx] <= '9'))
                {
                    hasNumbers = 1;
                }
            }

            if (hasCapitalEnglish && hasLowercaseEnglish && hasSpecialCharacters && hasNumbers)
            {
                flag2 = 1;
            }
            else
            {
                hasCapitalEnglish = 0;
                hasLowercaseEnglish = 0;
                hasNumbers = 0;
                hasSpecialCharacters = 0;
                printf("密码格式错误\n");
                sleep(1);
            }
        }

        /* 定义logOn登录行动并绑定数据 */
        json_object_object_add(logOnObj, "action", json_object_new_int(LOG_ON));
        json_object_object_add(logOnObj, "账号", json_object_new_string(account));
        json_object_object_add(logOnObj, "密码", json_object_new_string(password));
        const char* str = json_object_to_json_string(logOnObj);

        memset(sendBuf, 0, sizeof(sendBuf));
        strncpy(sendBuf, str, sizeof(sendBuf) - 1);
        /* 将json格式的字符串发送给服务端 */
        write(sockfd, sendBuf, sizeof(sendBuf));

        /* 等待服务器回应 */
        memset(recvBuf, 0, sizeof(sendBuf));
        read(sockfd, recvBuf, sizeof(recvBuf));
        /* 服务端如果确认账号密码错误，则返回的字符串信息为checkError */
        /* 如果服务端确认账号密码正确，但是已在线，则返回onLine */
        /* 服务端如果确认账号密码正确，则返回的字符串信息包含登录成功和账号的用户信息 */
        if(strncmp(recvBuf, "checkError", strlen("checkError")) == 0)
        {
            /* 失败 */
            printf("账号密码错误\n");
            sleep(1);
            continue;
        }
        else if(strncmp(recvBuf, "onLine", strlen("onLine")) == 0)
        {
            /* 失败 */
            printf("该用户已在线\n");
            sleep(1);
            //system("clear");
            continue;
        }
        else
        {//程序执行到这里说明服务端传回的是用户信息存放在readBuf中
            /* 成功 */
            printf("登陆成功\n");
            true = 1;
        }

    }
    
    struct json_object* userDataObj = json_tokener_parse(recvBuf);
    memset(MyData, 0, sizeof(userData));
    /* 解析 */
    struct json_object * ID_Obj = json_object_object_get(userDataObj, "ID");
    struct json_object * Name_Obj = json_object_object_get(userDataObj, "NAME");
    struct json_object * AGE_Obj = json_object_object_get(userDataObj, "AGE");
    struct json_object * SEX_Obj = json_object_object_get(userDataObj, "SEX");
    struct json_object * PASSWORD_Obj = json_object_object_get(userDataObj, "PASSWORD");
    /* 转换成C格式 */
    const char* ID = json_object_get_string(ID_Obj);
    const char* NAME = json_object_get_string(Name_Obj);
    int AGE = json_object_get_int(AGE_Obj);
    const char* SEX = json_object_get_string(SEX_Obj);
    const char* PASSWORD = json_object_get_string(PASSWORD_Obj);
    /* 赋给MyData */
    strncpy(MyData->ID, ID, (sizeof(char) * 15) - 1);
    strncpy(MyData->NAME, NAME, (sizeof(char) * 20) - 1);
    MyData->AGE = AGE;
    strncpy(MyData->SEX, SEX, (sizeof(char) * 5) - 1);
    strncpy(MyData->PASSWORD, PASSWORD, (sizeof(char) * 20) - 1);

    json_object_put(userDataObj);
    json_object_put(logOnObj);
    return ret;
}


/* 退出登录 */
int logOut(int sockfd)
{
    char sendBuf[COMMUNICATION_SIZE];
    memset(sendBuf, 0, sizeof(sendBuf));

    char recvBuf[COMMUNICATION_SIZE];
    memset(recvBuf, 0, sizeof(recvBuf));
    /* 创建action--LOG_OUT键值的json对象 */
    struct json_object* logOutObj = json_object_new_object();
    
    json_object_object_add(logOutObj, "action", json_object_new_int(LOG_OUT));
    //json_object_object_add(logOutObj, "contain", json_object_new_string("hello"));
    
    const char* str = json_object_to_json_string(logOutObj);

    
    strncpy(sendBuf, str, sizeof(sendBuf) - 1);
    
    /* 发送LOG_OUT行动给服务器 */
    
    write(sockfd, sendBuf, sizeof(sendBuf));
    
    printf("已退出登录");

    json_object_put(logOutObj);
    return 0;
}


/* 添加好友 */
int AddFriend(int sockfd, char* MyAccount)
{
    char account[BUFFER_SIZE];
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];
    struct json_object* FrAddObj = json_object_new_object();

    int true = 0;
    while(!true)
    {
        int flag = 0;
        int flag1 = 1;
        /* 获取纯数字的账号 */
        while (flag == 0)
        {
            flag = 1;
            memset(account, 0, sizeof(char) * BUFFER_SIZE);
            system("clear");
            printf("对方账号:");
            scanf("%s", account);
            if (strlen(account) != 10)
            {
                system("clear");
                if (flag1 == 1)
                {
                    printf("请输入10位整数\n");
                }
                else
                {
                    printf("小只因：不识字吗?\n");
                }
                flag1 = 0;
                flag = 0;
                sleep(1);
                
            }
            else
            {
                for (int idx = 0; idx < strlen(account); idx++)
                {
                    if (account[idx] < '0' || account[idx] > '9')
                    {
                        printf("无效的输入\n");
                        sleep(1);
                        flag = 0;
                        break;
                    }
                }
            }
            
            if(flag == 1)
            {
                if(strncmp(account, MyAccount, sizeof(char) * 10) == 0)
                {
                    printf("请不要输入您自己的账号\n");
                    sleep(1);
                    flag = 0;
                }
            }
            while (getchar() != '\n');//
        }

        /* 定义logOn登录行动并绑定数据 */
        json_object_object_add(FrAddObj, "action", json_object_new_int(ADD_FRIENDS));
        json_object_object_add(FrAddObj, "INVITER", json_object_new_string(MyAccount));//我是邀请者
        json_object_object_add(FrAddObj, "INVITEE", json_object_new_string(account));//被邀请者
        const char* str = json_object_to_json_string(FrAddObj);

        memset(sendBuf, 0, sizeof(sendBuf));
        strncpy(sendBuf, str, sizeof(sendBuf) - 1);

        write(sockfd, sendBuf, sizeof(sendBuf));

        memset(recvBuf, 0, sizeof(recvBuf));
        read(sockfd, recvBuf, sizeof(recvBuf));

        if(strncmp(recvBuf, "NotExists", strlen("NotExists")) == 0)
        {
            /* 账号不存在 */
            printf("该账号不存在\n");
            sleep(1);
        }
        else if(strncmp(recvBuf, "IsFriend", strlen("IsFriend")) == 0)
        {
            printf("您和对方已经是好友了\n");
            sleep(1);
        }   
        else
        {
            /* 发送邀请成功 */
            printf("好友邀请发送成功\n");
            true = 1;
        }
    }

    json_object_put(FrAddObj);
    return 0;
}

/* 查看我发出去的好友邀请处理结果 */
int viewMyInvite(int sockfd)
{   
    //SELECT * FROM FRIEND_DATA WHERE (INVITER = '我' AND DEAL != '好友');
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    struct json_object* viewMyInvites = json_object_new_object();

    /* 定义logOn登录行动并绑定数据 */
    json_object_object_add(viewMyInvites, "action", json_object_new_int(VIEW_MY_INVITE));
    const char* str = json_object_to_json_string(viewMyInvites);

    memset(sendBuf, 0, sizeof(sendBuf));
    strncpy(sendBuf, str, sizeof(sendBuf));
    write(sockfd, sendBuf, sizeof(sendBuf));

    while(1)
    {
        memset(recvBuf, 0, sizeof(recvBuf));
        read(sockfd, recvBuf, sizeof(recvBuf));

        /* 服务器告诉客户端读取完毕，可以停止读了 */
        if(strncmp(recvBuf, "NotInvite", strlen("NotInvite")) == 0)
        {

            printf("您暂无发送给他人的好友邀请\n");
            break;//进入下一步客户端的执行
        }
        else
        {
            /* 服务器告诉客户端读取完毕，可以停止读了 */
            if(strncmp(recvBuf, "FINISH", strlen("FINISH")) == 0)
            {
                break;//进入下一步客户端的执行
            }
            else
            {
                /* 程序在这里执行说明服务端传来的是验证消息，打印 */
                printf("%s\n", recvBuf);
            }
        }

    }

    json_object_put(viewMyInvites);
    return 0;   
}

/* 查看和处理请求加我为好友的验证消息 */
int viewOtherInvite(int sockfd)
{
    /* 客户端已经存放着账号和密码 */
    /* SELECT * FROM FRIEND_DATA WHERE INVITEE = 'MyAccount' AND DEAL = '正在考虑' */

    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    struct json_object* viewOtherInvites = json_object_new_object();
    /* 定义logOn登录行动并绑定数据 */
    json_object_object_add(viewOtherInvites, "action", json_object_new_int(VIEW_OTHER_INVITE));
    const char* str = json_object_to_json_string(viewOtherInvites);

    memset(sendBuf, 0, sizeof(sendBuf));
    strncpy(sendBuf, str, sizeof(sendBuf));
    write(sockfd, sendBuf, sizeof(sendBuf));

    int NumsChoice = 0;
    int flag = 0;
    int nums = 0;/* 用于记录有多少条好友请求 */
    /* 接收和打印服务端发来的数据 */
    while(1)
    {
        memset(recvBuf, 0, sizeof(recvBuf));
        read(sockfd, recvBuf, sizeof(recvBuf));
        if(strncmp(recvBuf, "NotVerifyMessage", strlen("NotVerifyMessage")) == 0)
        {
            printf("暂无他人向您发起好友请求\n");
            break;/* 从这跳出，flag未被修改 */
        }
        else
        {   
            /* 服务器告诉客户端读取完毕，可以停止读了 */
            if(strncmp(recvBuf, "FINISH", strlen("FINISH")) == 0)
            {
                flag = 1;/* 从这里跳出，flag被修改为1 */
                break;//进入下一步客户端的执行
            }
            else
            {
                /* 程序在这里执行说明服务端传来的是验证消息，打印 */
                printf("%s\n", recvBuf);
                nums++;
            }
        }
    }

    /* 判断从哪种情况跳出来的 */
    if(flag == 1)
    {
        /* 从这里跳出说明有验证消息,处理请求 */
        char tmpbuf[BUFFER_SIZE];
        int flag1 = 0;
        
        while(flag1 == 0)
        {
            flag1 = 1;
            memset(tmpbuf, 0, sizeof(tmpbuf));
            printf("输入要处理的编号：\n");
            scanf("%s", tmpbuf);
            for(int idx = 0; idx < strlen(tmpbuf); idx++)
            {
                if(tmpbuf[idx] < '0' || tmpbuf[idx] > '9')
                {
                    printf("无效输入\n");
                    flag1 = 0;
                    break;
                }
            }

            if(flag1 == 1)
            {
                /* 程序运行到这里说明是有效输入,但是可能不在验证消息选项编号内 */
                NumsChoice = strtol(tmpbuf, NULL, 10);
                if(NumsChoice > nums || NumsChoice <= 0)
                {
                    printf("无效输入\n");
                    flag1 = 0;
                }
                /* 如果没进入if，则输入时合法且在编号范围内的，此时flag1 = 1 */
            }
        }


        int choice = -1;
        while(1)
        {   
            system("clear");
            printf("0.拒绝、1.同意:");
            scanf("%d", &choice);
            if(choice == 1 || choice == 0)
            {
                break;
            }
            printf("请输入正确的选项\n");
            sleep(1);
        }
        

        /* 程序走到这里是有效输入 */
        /* 将选择发送给服务端 */
        struct json_object* NumsChoiceObj = json_object_new_object();
        json_object_object_add(NumsChoiceObj, "NumsChoice", json_object_new_int(NumsChoice));
        json_object_object_add(NumsChoiceObj, "choice", json_object_new_int(choice));
        const char* str = json_object_to_json_string(NumsChoiceObj);

        /* 将客户端的选择发送给服务端 */
        memset(sendBuf, 0, sizeof(sendBuf));
        strncpy(sendBuf, str, sizeof(sendBuf) - 1);
        write(sockfd, sendBuf, sizeof(sendBuf));

        printf("处理成功\n");
        json_object_put(NumsChoiceObj);
    }    

    json_object_put(viewOtherInvites);
    return 0;   
}

/* 私聊和好友列表 */
/* 私聊和好友列表结合在一个模块 */
//SELECT INVITEE FROM FRIEND_DATA WHERE INVITER = '我';
int privateChat(int sockfd)
{
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    /* 打包行动 */
    struct json_object* friendObj = json_object_new_object();
    json_object_object_add(friendObj, "action", json_object_new_int(PRIVATE_CHAT));
    const char* sendStr = json_object_to_json_string(friendObj);

    memset(sendBuf, 0, sizeof(sendBuf));
    strncpy(sendBuf, sendStr, sizeof(sendBuf) - 1);
    
    /* 发送行动给服务端 */
    write(sockfd, sendBuf, sizeof(sendBuf));

    /* 计数 */
    int nums = 0;
    while(1)
    {
        memset(recvBuf, 0, sizeof(recvBuf));
        read(sockfd, recvBuf, sizeof(recvBuf));
    
        if(strncmp(recvBuf, "NotFriends", strlen("NotFriends")) == 0)
        {
            /* 无好友 */
            printf("您暂无好友，请先添加好友\n");
            break;
        }
        else
        {   
            /* 有好友，先接收好友列表 */
            if(strncmp(recvBuf, "FINISH", strlen("FINISH")) == 0)
            {   
                /* 读取结束 */
                break;
            }
            else
            {
                printf("%s\n", recvBuf);
                /* 这里的nums用于记录有多少个好友 */
                nums++;
            }
                        
        }

    }

    /* 在这里可以通过判断nums的大小来判断上面是从哪里跳出来的 */
    if(nums != 0)
    {
        //草泥马的老子不想写了
        //你妈的你还想聊天吗？
        /* 有好友的情况下，进行下一部操作 */
        int choice = -1;
        while(1)
        {
            printf("选择好友编号聊天(0退出):");
            scanf("%d", &choice);
            if(choice < 0 || choice > nums)
            {
                printf("\n弱智?\n");
            }
            else
            {
                break;
            }
        }
        
        if(choice != 0)
        {   
            /* 如果choice不为0，说明弱智客户想和好友聊天，然后我又要为弱智客户堆屎山 */
            /* 先告诉服务端弱智客户想和哪个好友聊天 */
            struct  json_object * choiceSend = json_object_new_object();
            json_object_object_add(choiceSend, "chatChoice", json_object_new_int(choice));
            const char* Str = json_object_to_json_string(choiceSend);

            memset(sendBuf, 0, sizeof(sendBuf));
            strncpy(sendBuf, Str, sizeof(sendBuf) - 1);
    
            /* 发送行动给服务端 */
            write(sockfd, sendBuf, sizeof(sendBuf));
            /* 读写分离发送消息和接收消息 */
            privateMsgChat(sockfd);

            /* 释放json对象 */
            json_object_put(choiceSend);
        }
        else
        {   /* choice = 0 */
            /* 程序运行到这里说明有好友但是弱智客户不想聊天 */
            /* 告诉服务端弱智客户不想聊天 */
            
            memset(sendBuf, 0, sizeof(sendBuf));    
            strncpy(sendBuf, "NO_CHAT", sizeof(sendBuf));
            write(sockfd, sendBuf, sizeof(sendBuf));
            /* 退出函数 */
        }
        
    }

    /* 释放json对象 */
    json_object_put(friendObj);
    return 0;
}

/* 读线程函数 */
void* readThread(void * arg)
{
    /* 线程分离 */
    pthread_detach(pthread_self());

    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    int sockfd = ((PTH_CONNECT *)arg)->sockfd;
    memset(sendBuf, 0, sizeof(sendBuf));
    /* 告诉服务器取消息 */
    strncpy(sendBuf, "!@#$%^*&^%$#@!_^@%#$#!", strlen("!@#$%^*&^%$#@!_^@%#$#!"));

    while(1)
    {
        sleep(1);
        if(((PTH_CONNECT *)arg)->stop != STOP)
        {
            /* ((PTH_CONNECT *)arg)->stop != STOP即传入线程函数的地址的值没有改变 */
            /* 不断通知服务端取消息 */
            write(sockfd, sendBuf, sizeof(sendBuf));
            memset(recvBuf, 0, sizeof(recvBuf));
            /* 读服务器传回的消息 */
            read(sockfd, recvBuf, sizeof(recvBuf));
            
            if(strncmp(recvBuf, "!@#$%^*&^%$#@!_^@%#$#!", strlen("!@#$%^*&^%$#@!_^@%#$#!")) != 0)
            {
                
                printf("\033[1;34;47m%s\033[0;0;0m\n", recvBuf);
            }
        }
        else
        {
            break;
        }
    }

    pthread_exit(NULL);
}

/* 发送和接收消息，读写分离 */
static int privateMsgChat(int sockfd)
{
    /* 需要读写分离 */
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    pthread_t tid;
    PTH_CONNECT pth_Conect;

    pth_Conect.sockfd = sockfd;
    pth_Conect.stop = CONTINUE;
    pthread_create(&tid, NULL, readThread, (void*)&pth_Conect);
    system("clear");
    while(1)
    {
        memset(sendBuf, 0, sizeof(sendBuf));
        //printf("输入(ESC退出):");
        scanf("%s", sendBuf);
        if(strlen(sendBuf) == 1 && sendBuf[0] == 27)
        {
            /* 输入了ESC键 */
            /* 停止读线程函数的read */
            pth_Conect.stop = STOP;
            memset(sendBuf, 0, sizeof(sendBuf));
            /* 告诉客户端可以停止读了 */
            strncpy(sendBuf, "!@%^&$@(#^)!@*+@$#$@", sizeof("!@%^&$@(#^)!@*+@$#$@"));
            write(sockfd, sendBuf, sizeof(sendBuf));
            break;
        }
        else
        {
            /* 要发的消息,传给服务端 */
            write(sockfd, sendBuf, sizeof(sendBuf));
        }
    }
    return 0;
}

/* 服务端处理客户端私聊消息 */
int dealPrivateChat(int acceptfd, char* user, char* Friend, MsgHash * msgHash, sqlite3* Data_Db, pthread_mutex_t* Hash_Mutx)
{
    //"!@#$%^*&^%$#@!_^@%#$#!"  取消息交流
    //"!@%^&$@(#^)!@*+@$#$@"  停止读
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    while(1)
    {   
        sleep(1);
        memset(sendBuf, 0, sizeof(sendBuf));
        memset(recvBuf, 0, sizeof(recvBuf));
        read(acceptfd, recvBuf, sizeof(recvBuf));
        if(strncmp(recvBuf, "!@#$%^*&^%$#@!_^@%#$#!", strlen("!@#$%^*&^%$#@!_^@%#$#!")) == 0)
        {
            /* 去消息队列中取接收者是客户端和发送者是指定好友的消息，发回给客户端 */
            /* 上锁 */
            pthread_mutex_lock(Hash_Mutx);
            int ret = hashMsgGet(msgHash, Data_Db, Friend, user, sendBuf);
            pthread_mutex_unlock(Hash_Mutx);
            if(ret == ON_SUCCESS)
            {
                //将取出的消息给客户端
                write(acceptfd, sendBuf, sizeof(sendBuf));
            }
            else
            {
                /* 告诉客户端无消息 */
                strncpy(sendBuf, "!@#$%^*&^%$#@!_^@%#$#!", strlen("!@#$%^*&^%$#@!_^@%#$#!"));
                write(acceptfd, sendBuf, sizeof(sendBuf));
            }
        }
        else if(strncmp(recvBuf, "!@%^&$@(#^)!@*+@$#$@", strlen("!@%^&$@(#^)!@*+@$#$@")) == 0)
        {
            /* 停止读 */
            break;
        }
        else
        {
            printf("%s：%s\n", user, recvBuf);
            /* 聊天消息 */
            /* 往消息队列中存放消息，接收者为聊天对象 */
            /* 上锁 */
            pthread_mutex_lock(Hash_Mutx);
            hashMsgInsert(msgHash, user, Friend, recvBuf);
            pthread_mutex_unlock(Hash_Mutx);
        }
    }
    

    return 0;
}


/* 客户端创建群聊申请 */
int createGroup(int sockfd)
{
    /* 群聊以群聊名作为唯一key */
    /* 告诉客户端创建群聊，并将要创建的群聊名告知服务端 */
    /* 服务端将进行查重 */
    /* 如果该群已存在，告知已存在，重新输入 */
    /* 不存在创建成功 */
    char groupName[BUFFER_SIZE];
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];
    struct json_object * GroupNameCheck = json_object_new_object();
    
    while(1)
    {
        system("clear");
        printf("创建群聊名:");
        memset(groupName, 0, sizeof(groupName));
        scanf("%s", groupName);
        /* 打包 */
        json_object_object_add(GroupNameCheck, "action", json_object_new_int(CREATE_GROUP));
        json_object_object_add(GroupNameCheck, "GroupName", json_object_new_string(groupName));
        /* 生成字符串 */
        const char* sendStr = json_object_to_json_string(GroupNameCheck);
        memset(sendBuf, 0, sizeof(sendBuf));
        memset(recvBuf, 0, sizeof(recvBuf));
        /* 放入发送缓存区 */
        strncpy(sendBuf, sendStr, sizeof(sendBuf) - 1);
        /* 发送给服务端 */
        write(sockfd, sendBuf, sizeof(sendBuf));
        /* 等待服务端回应 */
        read(sockfd, recvBuf, sizeof(recvBuf));
        /* 判断回应 */
        if(strncmp(recvBuf, "SUCCESS", strlen("SUCCESS")) == 0)
        {
            /* 可行，跳出循环，执行下一步操作 */
            printf("创建成功\n");
            break;
        }
        else
        {
            /* 不可行，重新输入 */
            printf("该群名已存在\n");
            sleep(1);
        }
    }

    /* 释放 */
    json_object_put(GroupNameCheck);
    return 0;
}

/* 添加群聊 */
int AddGroup(int sockfd)
{
    /* 服务端先检查群聊是否存在 */
    /* 存在，查看是否已经是群成员 */

    char Group[COMMUNICATION_SIZE];
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    struct json_object * GroupObj = json_object_new_object();

    while(1)
    {
        system("clear");
        printf("群聊名(ESC退出):");
        memset(Group, 0, sizeof(Group));
        scanf("%s", Group);
        if(strlen(Group) == 1 && Group[0] == 27)
        {
            /* 不加了 */
            break;
        }
        
        /* 打包 */
        json_object_object_add(GroupObj, "action", json_object_new_int(ADD_GROUP));
        json_object_object_add(GroupObj, "GroupName", json_object_new_string(Group));
        /* 生成字符串 */
        const char* sendStr = json_object_to_json_string(GroupObj);
        memset(sendBuf, 0, sizeof(sendBuf));
        memset(recvBuf, 0, sizeof(recvBuf));
        /* 放入发送缓存区 */
        strncpy(sendBuf, sendStr, sizeof(sendBuf) - 1);
        /* 发送给服务端 */
        write(sockfd, sendBuf, sizeof(sendBuf));
        /* 等待服务端回应 */
        read(sockfd, recvBuf, sizeof(recvBuf));
        /* 判断回应 */
        if(strncmp(recvBuf, "ADDSUCCESS", strlen("ADDSUCCESS")) == 0)
        {
            /* 可行，跳出循环，执行下一步操作 */
            printf("加入成功\n");
            break;
        }
        else if(strncmp(recvBuf, "GROUP_NOT_EXISTS", strlen("GROUP_NOT_EXISTS")) == 0)
        {
            /* 不可行，重新输入 */
            printf("该群名不存在\n");
            sleep(1);
        }
        else
        {
            /* 已经是群成员 */
            printf("您已是群成员\n");
            sleep(1);
        }

    }

    /* 释放 */
    json_object_put(GroupObj);
    return 0;
}

/* 客户端:群聊和好友列表 */
int GroupChat(int sockfd)
{
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    /* 行动 */
    struct json_object * GrChatObj = json_object_new_object();
    json_object_object_add(GrChatObj, "action", json_object_new_int(GROUP_CHAT));
    const char* str = json_object_to_json_string(GrChatObj);

    memset(sendBuf, 0, sizeof(sendBuf));
    strncpy(sendBuf, str, sizeof(sendBuf) - 1);
    write(sockfd, sendBuf, sizeof(sendBuf));

    /* 计数 */
    int nums = 0;
    while(1)
    {
        /* 等待回应对 */
        memset(recvBuf, 0, sizeof(recvBuf));
        read(sockfd, recvBuf, sizeof(recvBuf));

        if(strncmp(recvBuf, "NotGroup", strlen("NotGroup")) == 0)
        {
            printf("您暂无群聊\n");
            break;
        }
        else
        {
            
            if(strncmp(recvBuf, "FINISH!@#$%^&*", strlen("FINISH!@#$%^&*")) == 0)
            {
                /* 读取完毕，执行下一步 */
                break;
            }
            else
            {
                /* 群名 */
                printf("%s\n", recvBuf);
                nums++;
            }
        }
    }

    if(nums != 0)
    {

        int choice = -1;
        while(1)
        {
            printf("选择群聊(0退出):");
            scanf("%d", &choice);
            if(choice < 0 || choice > nums)
            {
                printf("\n弱智?\n");
            }
            else
            {
                break;
            }
        }
        
        if(choice != 0)
        {   
            /* 如果choice不为0，说明弱智客户想和好友聊天，然后我又要为弱智客户堆屎山 */
            /* 先告诉服务端弱智客户想进入哪个群聊 */
            struct  json_object * choiceSend = json_object_new_object();
            json_object_object_add(choiceSend, "GroupChoice", json_object_new_int(choice));
            const char* sendStr = json_object_to_json_string(choiceSend);

            memset(sendBuf, 0, sizeof(sendBuf));
            strncpy(sendBuf, sendStr, sizeof(sendBuf) - 1);
            /* 发送行动给服务端 */
            write(sockfd, sendBuf, sizeof(sendBuf));
            /* 读写分离发送消息和接收消息 */
            GrpMsgChat(sockfd);

            /* 释放json对象 */
            json_object_put(choiceSend);
        }
        else
        {   /* choice = 0 */
            /* 程序运行到这里说明有好友但是弱智客户不想聊天 */
            /* 告诉服务端弱智客户不想聊天 */
            
            memset(sendBuf, 0, sizeof(sendBuf));    
            strncpy(sendBuf, "NO_CHAT", sizeof(sendBuf));
            write(sockfd, sendBuf, sizeof(sendBuf));
            /* 退出函数 */
        }
        
    }

    return 0;
}

/* 读线程函数 */
void* GrpReadThread(void * arg)
{
    /* 线程分离 */
    pthread_detach(pthread_self());

    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    int sockfd = ((PTH_CONNECT *)arg)->sockfd;
    memset(sendBuf, 0, sizeof(sendBuf));
    /* 告诉服务器取消息 */
    strncpy(sendBuf, "!@#$%^&*(+@!$@%&^)@$%@#}?", sizeof(sendBuf));

    while(1)
    {
        sleep(1);
        if(((PTH_CONNECT *)arg)->stop != STOP)
        {
            /* ((PTH_CONNECT *)arg)->stop != STOP即传入线程函数的地址的值没有改变 */
            /* 不断通知服务端取消息 */
            write(sockfd, sendBuf, sizeof(sendBuf));
            memset(recvBuf, 0, sizeof(recvBuf));
            /* 读服务器传回的消息 */
            read(sockfd, recvBuf, sizeof(recvBuf));
            
            if(strncmp(recvBuf, "!@#$%^&*(+@!$@%&^)@$%@#}?", strlen("!@#$%^&*(+@!$@%&^)@$%@#}?")) != 0)
            {
                printf("\033[1;34;47m%s\033[0;0;0m\n", recvBuf);
            }
        }
        else
        {
            break;
        }
    }

    pthread_exit(NULL);
}

/* 读写分离 */
static int GrpMsgChat(int sockfd)
{
    /* 需要读写分离 */
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];

    pthread_t tid;
    PTH_CONNECT pth_Conect;

    pth_Conect.sockfd = sockfd;
    pth_Conect.stop = CONTINUE;
    pthread_create(&tid, NULL, GrpReadThread, (void*)&pth_Conect);
    system("clear");
    while(1)
    {
        memset(sendBuf, 0, sizeof(sendBuf));
        //printf("输入(ESC退出):");
        scanf("%s", sendBuf);
        if(strlen(sendBuf) == 1 && sendBuf[0] == 27)
        {
            /* 输入了ESC键 */
            /* 停止读线程函数的read */
            pth_Conect.stop = STOP;
            memset(sendBuf, 0, sizeof(sendBuf));
            /* 告诉服务端可以停止读了 */
            strncpy(sendBuf, "^^^^&&&&*%$#!#%_+(){}?>{}", sizeof(sendBuf));
            write(sockfd, sendBuf, sizeof(sendBuf));

            break;
        }
        else
        {
            /* 要发的消息,传给服务端 */
            write(sockfd, sendBuf, sizeof(sendBuf));
        }
    }
    return 0;
}

/* 服务端:处理客户端群聊 */
int dealGrpChat(int acceptfd, char* user, char* Group, GpHash* Gp_Hash, sqlite3* Data_Db, pthread_mutex_t* Gp_Mutx, pthread_mutex_t* Db_Mutx)
{   
    /* 发：查询指定群聊中除用户外的其余成员，全部插一遍消息 */
    /* 取：取指定群聊，接收者是用户的消息，获取发送者的账号，再查询发送者的昵称 */
    char sendBuf[COMMUNICATION_SIZE];
    char recvBuf[COMMUNICATION_SIZE];
    char sender[ACCOUNT_SIZE];
    char sqlBuf[BUFFER_SIZE];
    while(1)
    {   
        memset(sendBuf, 0, sizeof(sendBuf));
        memset(recvBuf, 0, sizeof(recvBuf));
        char**GpResult = NULL;
        char* Errmsg;
        int Row = 0;
        int Columns = 0;
        read(acceptfd, recvBuf, sizeof(sendBuf));
        if(strncmp(recvBuf, "!@#$%^&*(+@!$@%&^)@$%@#}?", strlen("!@#$%^&*(+@!$@%&^)@$%@#}?")) == 0)
        {   
            memset(sender, 0, sizeof(sender));
            pthread_mutex_lock(Gp_Mutx);
            
            int ret = GpHashGet(Gp_Hash, Data_Db, Group, sender, user, sendBuf);
            pthread_mutex_unlock(Gp_Mutx);
            
            if(ret == ON_SUCCESS)
            {   
                printf("%s取到了\n", user);
                /* 查询发送者名字 */
                sprintf(sqlBuf, "SELECT NAME FROM USER_DATA WHERE ID = '%s'", sender);
                pthread_mutex_lock(Db_Mutx);
                sqlite3_get_table(Data_Db, sqlBuf, &GpResult, &Row, &Columns, &Errmsg);
                pthread_mutex_unlock(Db_Mutx);
                /* 将发送者名字和取到的消息拼接起来再发过去 */
                char Msg[200] = {0};
                strncpy(Msg, sendBuf, sizeof(Msg) - 1);
                sprintf(sendBuf, "[%s]:%s", GpResult[1], Msg);
                write(acceptfd, sendBuf, sizeof(sendBuf));
            }
            else
            {
                printf("没有取到\n");
                /* 告诉客户端无消息 */
                strncpy(sendBuf, "!@#$%^&*(+@!$@%&^)@$%@#}?", sizeof(sendBuf));
                write(acceptfd, sendBuf, sizeof(sendBuf));
            }
        }
        else if(strncmp(recvBuf, "^^^^&&&&*%$#!#%_+(){}?>{}", strlen("^^^^&&&&*%$#!#%_+(){}?>{}")) == 0)
        {   
            printf("%s退出群聊\n", user);
            /* 停止读 */
            break;
        }
        else
        {
            /* 聊天消息 */
            /* 查询指定群聊中除用户外其余的所有用户，全部发一遍 */
            
            sprintf(sqlBuf, "SELECT MEMBER FROM GROUP_DATA WHERE GROUP_NAME = '%s' AND MEMBER != '%s'", Group, user);
            /* 上锁查询 */
            pthread_mutex_lock(Db_Mutx);
            sqlite3_get_table(Data_Db, sqlBuf, &GpResult, &Row, &Columns, &Errmsg);
            pthread_mutex_unlock(Db_Mutx);
            /* 给群里的其余群员都发一遍 */
            int idx = 0;
            pthread_mutex_lock(Db_Mutx);
            for(idx = 1; idx <= Row; idx++)
            {
                GpHashInsert(Gp_Hash, Group, user, GpResult[idx], recvBuf);
            }
            pthread_mutex_unlock(Db_Mutx);
        }
        
        //sqlite3_free_table(GpResult);
    }
    
    return 0;
}

int sendFile()      //发送文件
{
    system("clear");
    char fileBuffer[BUFFER_SIZE];
    memset(fileBuffer, 0, sizeof(fileBuffer));

    printf("请输入要传输的文件名：\n");
    printf("(请输入用于功能测试的文件名：LjjsTXT.txt)\n");
    scanf("%s", fileBuffer);
    int fd1 = open(fileBuffer, O_RDONLY);
    if (fd1 == -1)
    {
        perror("该文件不存在");
        return 0;
    }

    int fd2 = open("接收到的文件.txt", O_WRONLY | O_CREAT, 0777);

    char buffer[BUFFER_SIZE];
    bzero(buffer, sizeof(buffer));

    int realyRead = 0;
    while (1)
    {
        realyRead = read(fd1, buffer, sizeof(buffer));
        if (realyRead == 0)
        {
            break;
        }

        write(fd2, buffer, realyRead);
        if (realyRead < sizeof(buffer))
        {
            break;
        }
    }
    return 0;
}
