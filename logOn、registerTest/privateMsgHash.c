#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "privateMsgHash.h"

/* 状态码 */
enum STATUS_CODE
{
    NOT_FIND = -1,
    ON_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    INVALID_ACCESS,
};

/* 映射函数, 返回值为keyId */
static int HashKeyId(char * recvAccount, int slotNums);

/* 初始化消息结点双链表 */
static int MsgListInit(MsgList* List);

/* 映射函数 */
static int HashKeyId(char * recvAccount, int slotNums)
{
    /* 将字符串账号转化为数字 */
    long int number = strtol(recvAccount, NULL, 10);
    /* 转换成的数字对数组大小取余 */
    int keyId = (number % slotNums);
    return keyId;
}

/* 初始化消息结点双链表 */
static int MsgListInit(MsgList* List)
{
    if(List == NULL)
    {
        return NULL_PTR;
    }

    List->head = (MsgNode*)malloc(sizeof(MsgNode) * 1);
    /* 虚拟头结点，不赋值 */
    List->head->pre = NULL;
    List->head->next = NULL;
    
    List->tail = NULL;
    List->MsgSize = 0;

    return 0;
}

/* 参数2为要定义的槽位数 */
int HashInit(MsgHash ** msgHash, int slotNum)
{
    MsgHash* hash = (MsgHash*)malloc(sizeof(MsgHash) * 1);
    if(hash == NULL)
    {
        return MALLOC_ERROR;
    }

    hash->slotNums = slotNum;
    hash->rcvKeyId = (MsgList*)malloc(sizeof(MsgList) * slotNum);
    
    /* 给每个链表初始化 */
    for(int idx = 0; idx < hash->slotNums; idx++)
    {
        MsgListInit(&(hash->rcvKeyId[idx]));
    }

    *msgHash = hash;
    
    return 0;
}

/* 插消息，插入后获取插入时的时间并插入 */
int hashMsgInsert(MsgHash * msgHash, char* Sender, char* Receiver, char Message)
{
    /* 获取 */
    int keyIdx = HashKeyId(Receiver, msgHash->slotNums);

    return 0;
}

/* 取消息，取完一个保存到数据库并释放 */
int hashMsgGet(MsgHash * msgHash, sqlite3* Data_Db, char* Sender, char* Receiver, char* Message)
{

    return 0;
}

/* 销毁消息哈希表 */
int hashMsgDel(MsgHash * msgHash)
{

    return 0;
}