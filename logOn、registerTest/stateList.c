#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stateList.h"

#define ACCOUNT_SIZE 10
#define NAME_SIZE 20



/* 初始化 */
int stateListInit(stateList ** list)
{
    int ret = 0;
    stateList * List = (stateList* )malloc(sizeof(stateList));
    memset(List, 0, sizeof(stateList));
    List->head = (stateNode*)malloc(sizeof(stateNode));
    List->head->next = NULL;
    List->head->pre = NULL;
    *list = List;
    return ret;
}

/* 新增结点:头插 */
int stateListInsert(stateList * list, char* Account, char * Name)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }

    stateNode* Node = (stateNode*)malloc(sizeof(stateNode));
    memset(Node, 0, sizeof(stateNode));
    strncpy(Node->account, Account, sizeof(char) * ACCOUNT_SIZE);
    strncpy(Node->name, Name, sizeof(char) * NAME_SIZE);
    
    Node->next = list->head->next;
    if(list->head->next != NULL)
    {
        list->head->next->pre = Node;
    }
    Node->pre = list->head;
    list->head->next = Node;

    return 0;
}

/* 查询账号是否在线，如果只想查看是否在线，Name可以传NULL,否则传入字符串指针 */
int stateListSearch(stateList* list, char* account, char* Name)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }
    stateNode* travelNode = list->head->next;

    while(travelNode != NULL)
    {
        if(strncmp(travelNode->account, account, sizeof(char)* ACCOUNT_SIZE) == 0)
        {
            if(Name != NULL)
            {
                strncpy(Name, travelNode->name, strlen(travelNode->name));
            }
            return SEARCH_SUCCESS;
        }
        travelNode = travelNode->next;
    }

    return NOT_FIND;
}

/* 删除数据 */
int stateListAppointValDel(stateList* list, char* buf)
{
    if(list == NULL)
    {
        return NULL_PTR;
    }
    stateNode* travelNode = list->head->next;

    while(travelNode != NULL)
    {
        if(strncmp(travelNode->account, buf, sizeof(char)* ACCOUNT_SIZE) == 0)
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

