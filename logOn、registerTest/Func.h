#ifndef _FUNC_H_
#define _FUNC_H_
#include "privateMsgHash.h"
#include "GrpMsgHash.h"
#include <sqlite3.h>

#define CONTINUE    0
#define STOP        1

#define MAX_LISTEN  128
#define COMMUNICATION_SIZE   256
#define SERVER_PORT 8081
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 256

typedef struct userData
{
    char ID[15];
    char NAME[20];
    int  AGE;
    char SEX[5];
    char PASSWORD[20];
}userData;

typedef struct PTH_CONNECT
{
    int sockfd;
    int stop;
}PTH_CONNECT;


/***************************************/
/* 注册账号查重 */
#define DUPLICATE_CHECK         0
/* 注册 */
#define REGISTER                1
/* 登录 */
#define LOG_ON                  2
/* 登陆时账号是否已在线检测 */
#define ONLINE_CHECK            3
/* 退出登录 */
#define LOG_OUT                 4
/* 私聊 */
#define PRIVATE_CHAT            5
/* 群聊 */
#define GROUP_CHAT              6
/* 加入群聊 */
#define ADD_GROUP               7
/* 添加好友 */
#define ADD_FRIENDS             8
/* 查看我发出的好友邀请结果 */
#define VIEW_MY_INVITE          9
/* 查看和处理别人给我发来的的好友邀请 */
#define VIEW_OTHER_INVITE       10
/* 删除好友 */
#define DEL_FRIENDS             11
/* 创建群聊 */
#define CREATE_GROUP            12
/* 删除我创建的群聊 */

/***************************************/


/* 服务端套接字创建函数，传出参数获取套接字描述符，第二个参数为端口号 */
int SrSocket(int * sockfdGet, int serverPort);

/* 客户端创建套接字函数，传出参数获取套接字描述符，第二个参数为服务端端口号，第三个参数为服务端IP */
int clnSocket(int * sockfdGet, int server_port, const char* server_ip);

/* 注册一个用户信息 */
int Register(int sockfd, char *buf);

/* 登录 */ /* 参数1：套接字文件描述符，参数2：用于接服务器传回的个人信息 */
int logon(int sockfd, userData* MyData);

/* 退出登录 */
int logOut(int sockfd);

/* 添加好友 */
int AddFriend(int sockfd, char* MyAccount);

/* 查看和处理请求加我为好友的验证消息 */
int viewOtherInvite(int sockfd);

/* 查看我发出去的好友邀请处理结果 */
int viewMyInvite(int sockfd);

/* 私聊和好友列表 */
/* 私聊和好友列表结合在一个模块 */
//SELECT INVITEE FROM FRIEND_DATA WHERE INVITER = '我';
int privateChat(int sockfd);

/* 服务端私聊处理客户端消息 */
int dealPrivateChat(int acceptfd, char* user, char* Friend, MsgHash * msgHash, sqlite3* Data_Db, pthread_mutex_t* Hash_Mutx);

/* 客户端创建群聊申请 */
int createGroup(int sockfd);

/* 添加群聊 */
int AddGroup(int sockfd);

/* 客户端:群聊和好友列表 */
int GroupChat(int sockfd);

/* 服务端:处理客户端群聊 */
int dealGrpChat(int acceptfd, char* user, char* Group, GpHash* Gp_Hash, sqlite3* Data_Db, pthread_mutex_t* Gp_Mutx, pthread_mutex_t* Db_Mutx);

#endif  //_FUNC_H_