#ifndef USERS_H
#define USERS_H


#include "../../server/include/server.h"

// three types, 0 when accepts, 1 when user or pass wrong for security, 2 when something else failed
typedef enum {
    USER_OK,
    USER_ERROR,
    GENERAL_ERROR
} user_status;

// Login a user
user_status login_user(const char * username, const char * password);

user_status logout_user(const char * username);

int find_user(const char * username);
bool add_user(char *s);


#endif