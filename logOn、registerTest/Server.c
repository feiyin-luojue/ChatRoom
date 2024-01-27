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

#define SERVER_PORT 8080
#define MAX_LISTEN  128
#define LOCAL_IPADDRESS "127.0.0.1"
#define BUFFER_SIZE 128
#define COMMUNICATION_SIZE   512



/****************** 




******************/

void* threadHandle(void* arg)
{   
    /* 设置线程分离 */
    pthread_detach(pthread_self());

    /* 要将acceptfd、用户状态列表等放在arg参数中 */
    int acceptfd = *(int*)arg;
        //接收缓存区
    char recvBuf[COMMUNICATION_SIZE];
    //发送缓存区
    char sendBuf[COMMUNICATION_SIZE];
    
    /* 线程的工作 */
    while(1)
    {   
        /* 读客户端信息，客户端和服务端读写缓存区统一为 512 */
        memset(recvBuf, 0, sizeof(recv));
        int readBytes = read(acceptfd, recvBuf, sizeof(recvBuf));
        if(readBytes == 0)//对于while中重复等待的read，这一步是不可少的，否则，将会继续往下执行，然后程序很明显就会报错了
        {
            printf("read readBytes == 0\n");
            close(acceptfd);
            break;
        }

        printf("%s\n", recvBuf);

        /* 分析客户端动作 */
        struct json_object* readObj = json_tokener_parse(recvBuf);//每次parse生成的json对象记得释放！！！

        /* 解析客户端的action */
        struct json_object* actionObj = json_object_object_get(readObj, "action");
        int action = json_object_get_int(actionObj);
#if 0
        if(strncmp(action, "duplicateCheck", strlen("duplicateCheck")) == 0)//账号注册查重行动
        {   //账号查重行动处理函数，to finish......
            memset(sendBuf, 0, sizeof(sendBuf));
            strncpy(sendBuf, "available", sizeof(sendBuf) - 1);
            write(acceptfd, sendBuf, sizeof(sendBuf));
        }
        else if(strncmp(action, "register", strlen("register")) == 0)//注册行动
        {   //注册行动处理函数，to finish......
            // memset(sendBuf, 0, sizeof(sendBuf));
            // strncpy(sendBuf, "registerSuccessful", sizeof(sendBuf) - 1);
            // write(acceptfd, sendBuf, sizeof(sendBuf));

        }
        else if(strncmp(action, "logOn", strlen("logOn")) == 0)//登录行动
        {   
            //解析出传来的账号密码
            //()先核对账号密码，如果账号密码不存在或者错误，返回checkError
            //如果账号密码正确，判断该账户是否已在线，如果在线，返回onLine
            //如果不在线，服务端处理登陆，返回用户的json格式字符串信息

            memset(sendBuf, 0, sizeof(sendBuf));
            strncpy(sendBuf, "", sizeof(sendBuf) - 1);
            write(acceptfd, sendBuf, sizeof(sendBuf));
        }
        else if(strncmp(action, "onLineCheck", strlen("onLineCheck")) == 0)//查询是否在线行动
        {

        }
        //to do......
#endif
        switch (action)
        {
            case DUPLICATE_CHECK://注册账号查重
                memset(sendBuf, 0, sizeof(sendBuf));
                strncpy(sendBuf, "available", sizeof(sendBuf) - 1);
                write(acceptfd, sendBuf, sizeof(sendBuf));
                break;

            case REGISTER ://注册
                memset(sendBuf, 0, sizeof(sendBuf));
                strncpy(sendBuf, "registerSuccessful", sizeof(sendBuf) - 1);
                write(acceptfd, sendBuf, sizeof(sendBuf));
                break;

            case LOG_ON ://登录
                memset(sendBuf, 0, sizeof(sendBuf));
                strncpy(sendBuf, "", sizeof(sendBuf) - 1);
                write(acceptfd, sendBuf, sizeof(sendBuf));
                break;

            case ONLINE_CHECK ://账号是否在线检测

            default:
                break;
        }

        json_object_put(readObj);
    }

    /* 线程退出 */
    pthread_exit(NULL);
}

/*************************/
pthread_mutex_t stateMutx;
stateList* List = NULL;    
sqlite3 * Data_db;
/*************************/


int main()
{

    int sockfd = 0;
    pthread_t tid;

    int ret = 0;

    /* 初始化在线用户列表 */
    stateListInit(&List);
    pthread_mutex_init(&stateMutx, NULL);

    ret = sqlite3_open("Data.db", &Data_db);


    if(ret != SQLITE_OK)
    {
        printf("sqlite3_open: %s\n", sqlite3_errmsg(Data_db));
        exit(1);
    }

    /* 创建服务端套接字 */
    SrSocket(&sockfd, SERVER_PORT);

    /* 客户的信息 */
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

        pthread_create(&tid, NULL, threadHandle, (void*)&acceptfd);
    }
    
    close(sockfd);
    return 0;
}