#ifndef _COMMON_H_
#define _COMMON_H_

#define HASH_KEY_SIZE   64
#define MESSAGE_SIZE    256
#define ACCOUNT_SIZE    15
#define ACCOUNT_LEN     10
#define TIME_SIZE       20
#define GROUP_SIZE      25
#define GROUP_LEN       20

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

#endif  //_COMMON_H_