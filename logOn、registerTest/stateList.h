#ifndef _STATE_LIST_H_
#define _STATE_LIST_H_

#define ACCOUNT_SIZE    15
#define NAME_SIZE       20
#define ON_LINE         1
#define GROUP_CHAT      2

typedef struct stateNode
{
    char account[15];
    int acceptfd;
    int state;//1在线，2群聊中

    struct stateNode* pre;
    struct stateNode* next;
}stateNode;

typedef struct stateList
{
    int ListSize;
    stateNode* head;//虚拟头节点
}stateList;

/* 状态码 */
enum STATUS_CODE
{
    NOT_FIND = -1,
    ON_SUCCESS,
    SEARCH_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    INVALID_ACCESS,
};


/* 初始化 */
int stateListInit(stateList ** list);

/* 新增用户状态, 默认上线 */
int stateListInsert(stateList * list, const char* Account, int Acceptfd);

/* 修改指定账号状态为在群聊中 */
int stateGroupChatModify(stateList * list, const char* Account);

/* 修改指定账号状态为仅在线上 */
int stateOnlineModify(stateList * list, const char* Account);

/* 查询账号是否在线,比对账号字符串 */
int stateListSearch(stateList* list, const char* Account);

/* 删除指定用户状态 */
int stateListAppointValDel(stateList* list, const char* Account);

/* 群发 */
// int stateListGroupSend(stateList* list, const char* sendBuf);

/* 销毁 */
int stateListDestroy(stateList * list);
#endif