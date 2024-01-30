#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stateList.h"




/* 修改用户状态 */
static int stateListModify(stateList * list, const char* Account, int State);

/* 初始化 */
int stateListInit(stateList ** list)
{
    int ret = 0;
    stateList * List = (stateList* )malloc(sizeof(stateList));
    memset(List, 0, sizeof(stateList));
    List->head = (stateNode*)malloc(sizeof(stateNode));
    List->head->next = NULL;
    List->head->pre = NULL;
    List->ListSize = 0;
    *list = List;
    return ret;
}

/* 新增用户状态 */
int stateListInsert(stateList * list, const char* Account, int Acceptfd)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }

    stateNode* Node = (stateNode*)malloc(sizeof(stateNode));
    memset(Node, 0, sizeof(stateNode));
    strncpy(Node->account, Account, sizeof(char) * ACCOUNT_SIZE);
    Node->acceptfd = Acceptfd;
    Node->state = ON_LINE;//登录时默认为1仅在线

    Node->next = list->head->next;
    if(list->head->next != NULL)
    {
        list->head->next->pre = Node;
    }
    Node->pre = list->head;
    list->head->next = Node;

    list->ListSize++;
    return 0;
}


/* 修改用户状态 */
static int stateListModify(stateList * list, const char* Account, int State)
{

    if(list == NULL)
    {
        return NULL_PTR;
    }
    stateNode* travelNode = list->head->next;
    while(travelNode != NULL)
    {
        if(strncmp(travelNode->account, Account, sizeof(char)* ACCOUNT_SIZE) == 0)
        {
            travelNode->state = State;
            return ON_SUCCESS;
        }
        travelNode = travelNode->next;
    }

    return NOT_FIND;
}

/* 修改指定账号状态为仅在线上 */
int stateOnlineModify(stateList * list, const char* Account)
{
    stateListModify(list, Account, ON_LINE);
    return 0;
}

/* 修改指定账号状态为在群聊中 */
int stateGroupChatModify(stateList * list, const char* Account)
{
    stateListModify(list, Account, GROUP_CHAT);
    return 0;
}


/* 查询账号是否在线,比对账号字符串 */
int stateListSearch(stateList* list, const char* Account)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }
    stateNode* travelNode = list->head->next;

    while(travelNode != NULL)
    {
        if(strncmp(travelNode->account, Account, sizeof(char)* ACCOUNT_SIZE) == 0)
        {
            return SEARCH_SUCCESS;
        }
        travelNode = travelNode->next;
    }

    return NOT_FIND;
}

/* 删除指定用户状态 */
int stateListAppointValDel(stateList* list, const char* Account)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }
    stateNode* travelNode = list->head->next;

    while(travelNode != NULL)
    {
        if(strncmp(travelNode->account, Account, sizeof(char)* ACCOUNT_SIZE) == 0)
        {
            travelNode->pre->next = travelNode->next;
            if(travelNode->next != NULL)
            {
                travelNode->next->pre = travelNode->pre; 
            }
            free(travelNode);
            list->ListSize--;
            return 0;
        }

        travelNode = travelNode->next;
    }

    return NOT_FIND;
}

/* 群发 */
int stateListGroupSend(stateList* list, const char* sendBuf)
{
    

    return 0;
}


/* 销毁 */
int stateListDestroy(stateList * list)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }

    stateNode * travelNode = list->head->next;
    /* 释放节点 */
    while(travelNode != NULL)
    {
        travelNode->pre->next = travelNode->next;
        if(travelNode->next != NULL)
        {
            travelNode->next->pre = travelNode->pre;
        }
        free(travelNode);
        travelNode = list->head->next;
    }
    /* 释放头节点 */
    if(list->head != NULL)
    {
        free(list->head);
    }
    /* 释放列表 */
    if(list != NULL)
    {
        free(list);
    }

    return 0;
}

