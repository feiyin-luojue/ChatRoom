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


#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 128
#define COMMUNICATION_SIZE 512


int main()
{
    int sockfd = 0;
    /* 创建客户端文件描述符 */
    clnSocket(&sockfd, SERVER_PORT, SERVER_IP);
    
    char buf[COMMUNICATION_SIZE];
    memset(buf, 0, sizeof(buf));
    /* 注册 */
    Register(sockfd, buf);
    printf("%s\n", buf);
    memset(buf, 0, sizeof(buf));
    /* 登录 */
    logon(sockfd, buf);
    printf("%s\n", buf);

    
    close(sockfd);
    return 0;
}