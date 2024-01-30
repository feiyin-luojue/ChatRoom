#ifndef _SQLITE3_
#define _SQLITE3_
#include <sqlite3.h>

/* 数据库创建用户信息表 */
int sqlite3UserTableCreate(sqlite3 * Data_db);

/* 数据库创建群聊成员信息表 */
int sqlite3GroupTableCreate(sqlite3 * Data_db);

/* 数据库创建好友关系表 */
int sqlite3FriendshipTableCreate(sqlite3 * Data_db);


#endif  //_SQLITE3_