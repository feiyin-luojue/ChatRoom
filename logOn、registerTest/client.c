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





int main()
{
    int sockfd = 0;
    /* 创建客户端文件描述符 */
    clnSocket(&sockfd, SERVER_PORT, SERVER_IP);

    /* 用于存放登陆后的用户信息 */
    struct json_object* userDataObj = NULL;
    userData MyData;//客户端用于登陆后保存个人信息的结构体

    char buf[COMMUNICATION_SIZE];
    memset(buf, 0, sizeof(buf));
    /* 注册 */
    //Register(sockfd, buf);
    //printf("%s\n", buf);
    //memset(buf, 0, sizeof(buf));
    /* 登录 */
    logon(sockfd, &MyData);
    printf("%s, %s, %d, %s, %s\n", MyData.ID, MyData.NAME, MyData.AGE, MyData.SEX, MyData.PASSWORD);

    printf("%s\n", buf);
    AddFriend(sockfd, MyData.ID);
    //viewOtherInvite(sockfd);
    //viewMyInvite(sockfd);
    sleep(30);

    logOut(sockfd);

    close(sockfd);
    return 0;
}