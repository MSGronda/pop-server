#ifndef USERS_H
#define USERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/args.h"


// three types, 0 when accepts, 1 when user or pass wrong for security, 2 when something else failed
typedef enum {
    USER_OK,
    USER_ERROR,
    GENERAL_ERROR
} user_status;


// Load users to system
void load_users( users_data users_array[], size_t amount_users);

// Login a user
user_status login_user(const char * username, const char * password);

// Close user management
int finish_users();

int find_user(const char * username);


#endif