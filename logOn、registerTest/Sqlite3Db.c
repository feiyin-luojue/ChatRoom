#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

/* 数据库创建用户信息表 */
int sqlite3UserTableCreate(sqlite3 * Data_db)
{
    char *sql = NULL;
    sql = "CREATE TABLE IF NOT EXISTS USERDATA("  \
          "ID             CHAR(15)       PRIMARY KEY     NOT NULL," \
          "NAME           CHAR(20)                       NOT NULL," \
          "AGE            INT                            NOT NULL," \
          "SEX            CHAR(3)," \
          "PASSWORD       CHAR(20));";


    int ret = sqlite3_exec(Data_db, sql, NULL, NULL, NULL);
    if(ret != SQLITE_OK)
    {
        printf("sqlite3_open: %s\n", sqlite3_errmsg(Data_db));
        exit(1);
    }
    
    return 0;
}