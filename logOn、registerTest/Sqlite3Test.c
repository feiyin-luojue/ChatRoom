#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>


int main()
{
    sqlite3 * db = NULL;
    int ret = sqlite3_open("Data.db", &db);
    char * sql = "SELECT INVITEE FROM FRIEND_DATA WHERE INVITER = '1111111111'"
}