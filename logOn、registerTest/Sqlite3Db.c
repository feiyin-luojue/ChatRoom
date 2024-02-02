#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

/* 数据库创建用户信息表 */
int sqlite3UserTableCreate(sqlite3 * Data_db)
{
    char *sql = NULL;
    sql = "CREATE TABLE IF NOT EXISTS USER_DATA("  \
          "ID             CHAR(15)       PRIMARY KEY     NOT NULL," \
          "NAME           CHAR(20)                       NOT NULL," \
          "AGE            INT                            NOT NULL," \
          "SEX            CHAR(3)," \
          "PASSWORD       CHAR(20));";


    int ret = sqlite3_exec(Data_db, sql, NULL, NULL, NULL);
    if(ret != SQLITE_OK)
    {
        printf("sqlite3_open1: %s\n", sqlite3_errmsg(Data_db));
        exit(1);
    }
    
    return 0;
}

/* 数据库创建群聊成员信息表 */
int sqlite3GroupTableCreate(sqlite3 * Data_db)
{
    char *sql = NULL;
    sql = "CREATE TABLE IF NOT EXISTS GROUP_DATA("  \
          "GROUP_NAME       CHAR(20), "\
          "MEMBER           CHAR(15));";

  
    int ret = sqlite3_exec(Data_db, sql, NULL, NULL, NULL);
    if(ret != SQLITE_OK)
    {
        printf("sqlite3_open2: %s\n", sqlite3_errmsg(Data_db));
        exit(1);
    }
    
    return 0;
}

/* 数据库创建好友关系表 */
int sqlite3FriendshipTableCreate(sqlite3 * Data_db)
{
    char *sql = NULL;
    sql = "CREATE TABLE IF NOT EXISTS FRIEND_DATA("  \
          "INVITER            CHAR(15),"\
          "INVITEE            CHAR(15),"\
          "DEAL               CHAR(15));";//DEAL为
  
    int ret = sqlite3_exec(Data_db, sql, NULL, NULL, NULL);
    if(ret != SQLITE_OK)
    {
        printf("sqlite3_open3: %s\n", sqlite3_errmsg(Data_db));
        exit(1);
    }
    
    return 0;
}


// int main() 
// {
//     sqlite3 *db;
//     char *zErrMsg = 0;
//     int rc;

//     rc = sqlite3_open(":memory:", &db);
//     if (rc) 
//     {
//         fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
//         return(0);
//     } 
//     else 
//     {
//         fprintf(stderr, "Opened database successfully\n");
//     }

//     // 获取当前时间
//     time_t rawtime;
//     struct tm * timeinfo;
//     char buffer[20];

//     time(&rawtime);
//     timeinfo = localtime(&rawtime);

//     strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

//     printf("Current time: %s\n", buffer);
//     int len = strlen(buffer);
//     printf("%d\n", len);
//     sqlite3_close(db);
//     return 0;
// }
//SELECT * FROM your_table ORDER BY your_datetime_column;

// now = time(NULL);
// ptm = localtime(&now);
// printf("%s\n", asctime(ptm));

// #include <stdio.h>
// #include <stdlib.h>
// #include <sqlite3.h>

// int main() {
//     sqlite3 *db;
//     char *zErrMsg = 0;
//     int rc;

//     // 打开数据库连接
//     rc = sqlite3_open(":memory:", &db);
//     if (rc) {
//         fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
//         return(0);
//     } else {
//         fprintf(stderr, "Opened database successfully\n");
//     }

//     // 创建表
//     char *sql = "CREATE TABLE IF NOT EXISTS your_table ("
//                 "id INTEGER PRIMARY KEY,"
//                 "data TEXT,"
//                 "inserted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

//     rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
//     if (rc != SQLITE_OK) {
//         fprintf(stderr, "SQL error: %s\n", zErrMsg);
//         sqlite3_free(zErrMsg);
//     } else {
//         fprintf(stdout, "Table created successfully\n");
//     }

//     // 插入数据
//     sql = "INSERT INTO your_table (data) VALUES ('Data 1');"
//           "INSERT INTO your_table (data) VALUES ('Data 2');"
//           "INSERT INTO your_table (data) VALUES ('Data 3');";

//     rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
//     if (rc != SQLITE_OK) {
//         fprintf(stderr, "SQL error: %s\n", zErrMsg);
//         sqlite3_free(zErrMsg);
//     } else {
//         fprintf(stdout, "Records created successfully\n");
//     }

//     // 查询并按插入时间排序输出数据
//     printf("Querying and ordering by insertion time:\n");
//     sql = "SELECT * FROM your_table ORDER BY inserted_at;";
//     rc = sqlite3_exec(db, sql, [](void *data, int argc, char **argv, char **azColName) -> int {
//         for (int i = 0; i < argc; i++) {
//             printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//         }
//         printf("\n");
//         return 0;
//     }, 0, &zErrMsg);

//     if (rc != SQLITE_OK) {
//         fprintf(stderr, "SQL error: %s\n", zErrMsg);
//         sqlite3_free(zErrMsg);
//     }

//     sqlite3_close(db);
//     return 0;
// }
