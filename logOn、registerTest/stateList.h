#ifndef _STATE_LIST_H_
#define _STATE_LIST_H_

typedef struct stateNode
{
    char account[15];
    char name[20];

    struct stateNode* pre;
    struct stateNode* next;
}stateNode;

typedef struct stateList
{
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

/* 新增结点:头插 */
int stateListInsert(stateList * list, char* Account, char * Name);

/* 查询账号是否在线，如果只想查看是否在线，Name可以传NULL,否则传入字符串指针 */
int stateListSearch(stateList* list, char* account, char* Name);

/* 删除数据 */
int stateListAppointValDel(stateList* list, char* buf);

/* 销毁 */
int stateListDestroy(stateList * list);

#endif