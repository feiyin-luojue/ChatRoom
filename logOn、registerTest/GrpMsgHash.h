#ifndef _GRP_MSG_HASH_H_
#define _GRP_MSG_HASH_H_

#include "common.h"

/* 消息结点 */
typedef struct GpNode
{
    /* 消息数据 */
    char messge[MESSAGE_SIZE];
    char sender[ACCOUNT_SIZE];
    char receiver[ACCOUNT_SIZE];/* 通过接收者账号给节点取数组索引 */
    
    char time[TIME_SIZE];

    struct MsgNode* pre;
    struct MsgNode* next;
}GpNode;

#endif //_GRP_MSG_HASH_H_