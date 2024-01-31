#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stateList.h"


int main()
{
    stateList * list = NULL;
    stateListInit(&list);
    // char buf[10] = {"1234567891"};
    // stateListInsert(list, buf, 1, 1);
    char buf1[10] = {"1234567892"};

    stateListInsert(list, buf1, 1);
    int ret = stateListSearch(list, buf1);
    if(ret == SEARCH_SUCCESS)
    {
        printf("exists\n");
    }
    /* 删除指定用户状态 */
    stateListAppointValDel(list, buf1);
    ret = stateListSearch(list, buf1);
    if(ret == SEARCH_SUCCESS)
    {
        printf("exists\n");
    }
    stateListInsert(list, buf1, 1);
    ret = stateListSearch(list, buf1);
    if(ret == SEARCH_SUCCESS)
    {
        printf("exists\n");
    }
    // stateListAppointValDel(list, buf);
    // ret = stateListSearch(list, buf);
    // if(ret == SEARCH_SUCCESS)
    // {
    //     printf("exists\n");
    // }
    // ret = stateListSearch(list, buf1);
    // if(ret == SEARCH_SUCCESS)
    // {
    //     printf("exists\n");
    // }
    stateListDestroy(list);
}