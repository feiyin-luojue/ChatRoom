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

#define SERVER_PORT 8080
#define MAX_LISTEN  128
#define LOCAL_IPADDRESS "127.0.0.1"
#define BUFFER_SIZE 128
#define COMMUNICATION_SIZE   512



int main()
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
    localAddress.sin_port = htons(SERVER_PORT);
    /* ip地址需要转成大端 */
    /* Address to accept any incoming messages.  */
    /* INADDR_ANY = 0x00000000 */
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY); 

    /* 绑定 */
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

    /* 客户的信息 */
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(struct sockaddr_in));

    /* 接收客户端的连接 */
    socklen_t clientAddressLen = 0;
    int acceptfd = accept(sockfd, (struct sockaddr *)&clientAddress, &clientAddressLen);
    if (acceptfd == -1)
    {
        perror("accpet error");
        exit(-1);
    }

    //接收缓存区
    char recvBuf[COMMUNICATION_SIZE];
    //发送缓存区
    char sendBuf[COMMUNICATION_SIZE];

while(1)
{ 
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
    const char* action = json_object_get_string(actionObj);
    
    if(strncmp(action, "duplicateCheck", strlen("logOn")) == 0)//账号查重行动
    {   //账号查重行动处理函数，to finish......
        memset(sendBuf, 0, sizeof(sendBuf));
        strncpy(sendBuf, "available", sizeof(sendBuf) - 1);
        write(acceptfd, sendBuf, sizeof(sendBuf));
    }
    else if(strncmp(action, "register", strlen("logOn")) == 0)//注册行动
    {   //注册行动处理函数，to finish......
        memset(sendBuf, 0, sizeof(sendBuf));
        strncpy(sendBuf, "registerSuccessful", sizeof(sendBuf) - 1);
        write(acceptfd, sendBuf, sizeof(sendBuf));
    }
    else if(strncmp(action, "logOn", strlen("logOn")) == 0)//登录行动
    {   //登录行动处理函数，to finish......
        memset(sendBuf, 0, sizeof(sendBuf));
        strncpy(sendBuf, "{ \"昵称\": \"tjl123456\", \"性别\": \"男\", \"年龄\": 18, \"账号\": \"1111111111\", \"密码\": \"Tjl123456!\", \"地址\": \"湖南\", \"个性签名\": \"这是一个个性签名\" }", sizeof(sendBuf) - 1);
        write(acceptfd, sendBuf, sizeof(sendBuf));
    }
    //to do......


    json_object_put(readObj);
}

    close(sockfd);
    return 0;
}