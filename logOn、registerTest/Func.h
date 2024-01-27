#ifndef _FUNC_H_
#define _FUNC_H_


#define MAX_LISTEN  128
#define COMMUNICATION_SIZE   256
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 128


#define DUPLICATE_CHECK     0
#define REGISTER            1
#define LOG_ON              2
#define ONLINE_CHECK        3
#define LOG_OUT             4

/* 服务端套接字创建函数，传出参数获取套接字描述符，第二个参数为端口号 */
int SrSocket(int * sockfdGet, int serverPort);

/* 客户端创建套接字函数，传出参数获取套接字描述符，第二个参数为服务端端口号，第三个参数为服务端IP */
int clnSocket(int * sockfdGet, int server_port, const char* server_ip);

/* 新建一个用户信息 */
int Register(int sockfd, char *buf);

/* 登录 */ /* 参数1：套接字文件描述符，参数2：用于接服务器传回的个人信息 */
int logon(int sockfd, char* buf);

/* 退出登录 */
int logOut(int sockfd);

#endif  //_FUNC_H_