#ifndef _PRIVATE_MSG_HASH_H_
#define _PRIVATE_MSG_HASH_H_

#define MESSAGE_SIZE    256
#define ACCOUNT_SIZE    15
#define ACCOUNT_LEN     10
#define TIME_SIZE       20

/* 消息结点 */
typedef struct MsgNode
{
    /* 消息数据 */
    char messge[MESSAGE_SIZE];
    char sender[ACCOUNT_SIZE];
    char receiver[ACCOUNT_SIZE];/* 通过接收者账号给节点取数组索引 */
    char time[TIME_SIZE];

    struct MsgNode* pre;
    struct MsgNode* next;
}MsgNode;

/* 消息结点双链表 */
typedef struct MsgList
{
    MsgNode* head;//虚拟头节点，分配空间
    MsgNode* tail;//标记，不分配
    /* 链表结点个数 */
    int MsgSize;
}MsgList;

typedef struct MsgHash
{
    /* 数组位置数 */
    int slotNums;
    /* 指向连续的消息结点双链表空间 */
    MsgList* rcvKeyId;
}MsgHash;


#endif //_PRIVATE_MSG_HASH_H_