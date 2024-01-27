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
#include <json-c/arraylist.h>
#include <json-c/json.h>
#include "Func.h"


#define MAX_LISTEN  128
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 128
#define COMMUNICATION_SIZE 512

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
        if (strncmp(recvBuf, "available", strlen("available")) == 0)
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

    int sexchoice = 0;
    while (1)
    {
        system("clear");
        printf("选择性别(1.男 2.女):");
        scanf("%d", &sexchoice);
        /* 清空缓存区 */
        while (getchar() != '\n');

        if (sexchoice == 1)
        {
            strncpy(sex, "男", sizeof(sex) - 1);
            break;
        }

        if (sexchoice == 2)
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

/* 用户注册密码 */
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
    }

    /* 将字符串复制给传进的字符串数组 */
    strncpy(buf, str, (sizeof(char) * COMMUNICATION_SIZE) - 1);

    /* 释放json对象 */
    json_object_put(accountObj);
    json_object_put(userDataSend);

    return ret;
}


/* 登录 */ /* 参数1：套接字文件描述符，参数2：用于接服务器传回的个人信息 */
int logon(int sockfd, char* buf)
{
    int ret = 0;
    int true = 0;
    struct json_object* logOnObj = json_object_new_object();
    struct json_object* rplyObj = json_object_new_object();
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

        /* 定义logOn注册行动并绑定数据 */
        json_object_object_add(logOnObj, "action", json_object_new_int(LOG_ON));
        json_object_object_add(logOnObj, "账号", json_object_new_string(account));
        json_object_object_add(logOnObj, "密码", json_object_new_string(password));
        const char* str = json_object_to_json_string(logOnObj);


        memset(sendBuf, 0, sizeof(sendBuf));
        strncpy(sendBuf, str, sizeof(sendBuf) - 1);
        /* 将json格式的字符串发送给客户端 */
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
            continue;
        }
        else if(strncmp(recvBuf, "onLine", strlen("onLine")) == 0)
        {
            /* 失败 */
            printf("该用户已在线");
            continue;
        }
        else
        {//程序执行到这里说明服务端传回的是用户信息存放在readBuf中
            /* 成功 */
            printf("登陆成功");
            true = 1;
        }

    }
    
    /* 将客户端字符串复制给传进的字符串数组 */
    strncpy(buf, recvBuf, (sizeof(char) * COMMUNICATION_SIZE) - 1);
    
    json_object_put(logOnObj);
    return ret;
}

