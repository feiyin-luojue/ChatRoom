#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <error.h>
#include <json-c/json.h>


#define BUFFER_SIZE     128
#define USERDATA_SIZE   512
#define ACCOUNT_SIZE    10


/* 新建一个用户信息 */
void Register(int sockfd)
{
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
    /* 地址 */
    char address[BUFFER_SIZE];
    memset(address, 0, sizeof(char) * BUFFER_SIZE);
    /* 个性签名 */
    char personalSign[BUFFER_SIZE];
    memset(personalSign, 0, sizeof(char) * BUFFER_SIZE);
    


    //while(1)
    //{
        int flag = 0;
        int flag1 = 1;
        /* 获取纯数字的账号 */
        while(flag == 0)
        {   
            flag = 1;
            memset(account, 0, sizeof(char) * BUFFER_SIZE);
            system("clear");
            printf("账号(10位整数):");
            scanf("%s", account);
            if(strlen(account) != 10)
            {
                system("clear");
                if(flag1 == 1)
                {
                    printf("请输入10位整数\n");
                }
                else
                {
                    printf("小只因：听不懂人话吗?\n");
                }
                flag1 = 0;
                flag = 0;
                sleep(1);
                while(getchar() != '\n');
            }
            else
            {
                for(int idx = 0; idx < strlen(account); idx++)
                {
                    if(account[idx] < '0' || account[idx] > '9')
                    {
                        printf("无效的输入\n");
                        sleep(1);
                        flag = 0;
                        break;
                    }
                }
            }
        }
    
        /* 向服务端发起确认账号是否已存在 */
        /* to do...... */
        
        //write(sockfd, account, sizeof(char) * BUFFER_SIZE);
        
        //read(sockfd, )
    //}

    system("clear");
    printf("昵称：");
    scanf("%s", name);

    int sexchoice = 0;
    while(1)
    {   
        system("clear");
        printf("选择性别(1.男 2.女):");
        scanf("%d",&sexchoice);
        /* 清空缓存区 */
        while(getchar() != '\n');

        if(sexchoice == 1)
        {
            strncpy(sex, "男", sizeof(sex) - 1);
            break;
        }

        if(sexchoice == 2)
        {
            strncpy(sex, "女", sizeof(sex) - 1);
            break;
        }

        printf("小只因：没长眼睛？\n");
        sleep(1);
    }
    
    char tmp[BUFFER_SIZE];
    /* age为int型，防止输入字符型出错，保证用户输入为int型 */
    while(1)
    {   
        memset(tmp, 0, sizeof(char) * BUFFER_SIZE);
        system("clear");
        printf("年龄：");
        scanf("%s", tmp);
        if(sscanf(tmp, "%d", &age) == 1 && age > 0 && age <= 100)
        {
            if(age < 23)
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
    
    while(getchar() != '\n');
    system("clear");
    printf("密码：");
    scanf("%s", password);
    printf("小只因：我知道你的密码咯!\n");
    sleep(1);

    system("clear");
    printf("地址：");
    scanf("%s", address);

    system("clear");
    printf("个性签名：");
    scanf("%s", personalSign);

    struct json_object* userData = json_object_new_object();
    const char* str = NULL;
    json_object_object_add(userData, "昵称", json_object_new_string(name));
    json_object_object_add(userData, "性别", json_object_new_string(sex));
    json_object_object_add(userData, "年龄", json_object_new_int(age));
    json_object_object_add(userData, "账号", json_object_new_string(account));
    json_object_object_add(userData, "密码", json_object_new_string(password));
    json_object_object_add(userData, "地址", json_object_new_string(address));
    json_object_object_add(userData, "个性签名", json_object_new_string(personalSign));

    str = json_object_to_json_string(userData);

    system("clear");

    printf("注册成功！您的用户信息：%s\n", str);

    char Data[USERDATA_SIZE];
    memset(Data, 0, sizeof(Data));
    
    /* 存放最终的的用户信息 */
    strncpy(Data, str, sizeof(Data) - 1);

    json_object_put(userData);
}







int main(void) {
    // 创建JSON对象
    struct json_object* userDataBase = json_object_new_object();
    const char* str = NULL;

    /* 打包一个用户信息 */
    struct json_object* USER1 = json_object_new_object();
    char tmpbuf[30] = {0};
    scanf("%s", tmpbuf);
    json_object_object_add(USER1, "name", json_object_new_string(tmpbuf));
    json_object_object_add(USER1, "age", json_object_new_int(20));
    json_object_object_add(USER1, "ID", json_object_new_string("1356026577"));
    json_object_object_add(USER1, "password", json_object_new_string("tjl123456"));
    json_object_object_add(USER1, "个性签名", json_object_new_string("这是一个个性签名"));


    /* 向总的userData中添加单个用户对象的信息键值对 */
    json_object_object_add(userDataBase, "1356026577", USER1);
    str = json_object_to_json_string(userDataBase);

    printf("%ld\n", strlen(str));
    printf("%s\n", str);


    /* 修改信息 */
    struct json_object * tmp = NULL;
    /* 提取出值 */
    json_object_object_get_ex(userDataBase, "1356026577", &tmp);
    /* 修改 */
    json_object_object_add(tmp, "name", json_object_new_string("xiaoming"));

    str = json_object_to_json_string(userDataBase);

    printf("%ld\n", strlen(str));
    printf("%s\n", str);

    // 释放JSON对象内存
    json_object_put(userDataBase);

    return 0;
}
