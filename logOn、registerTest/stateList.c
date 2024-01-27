#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stateList.h"





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
int stateListInsert(stateList * list, char* Account, int Acceptfd, int State)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }

    stateNode* Node = (stateNode*)malloc(sizeof(stateNode));
    memset(Node, 0, sizeof(stateNode));
    strncpy(Node->account, Account, sizeof(char) * ACCOUNT_SIZE);
    Node->acceptfd = Acceptfd;
    Node->state = State;

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
int stateListModify(stateList * list, char* Account, int State)
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

/* 查询账号是否在线 */
int stateListSearch(stateList* list, char* Account)
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
int stateListAppointValDel(stateList* list, char* Account)
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
                free(travelNode);
                return 0;
            }
            
        }

        travelNode = travelNode->next;
    }

    return NOT_FIND;
}

/* 销毁 */
int stateListDestroy(stateList * list)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }

    stateNode * travelNode = list->head->next;

    while(travelNode != NULL)
    {
        travelNode->pre->next = travelNode->next;
        if(travelNode->next != NULL)
        {
            travelNode->next->pre = travelNode->pre;
        }
        if(travelNode != NULL)
        {
            free(travelNode);
        }
        
        travelNode = list->head->next;
    }

    if(list->head != NULL)
    {
        free(list->head);
    }

    if(list != NULL)
    {
        free(list);
    }

    return 0;
}

