#ifndef USERS_H
#define USERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USER_PASS_MAX_LONG 255
// m^3 (marcos,marcos,max)
#define STARTING_USERS 3
#define USER_FILE "names.txt"
#define DEFAULT_READ "r"


typedef struct {
    char username[USER_PASS_MAX_LONG+1]; //long + \0
    char password[USER_PASS_MAX_LONG+1];
} user_data;

// three types, 0 when accepts, 1 when user or pass wrong for security, 2 when something else failed
typedef enum {
    USER_OK,
    USER_ERROR,
    GENERAL_ERROR
} user_status;


int initialize_users(const char * usersFile);

// Load users to system
int load_users();

// Login a user
user_status login_user(const char * username, const char * password);

// Close user management
int finish_users();

int find_user(const char * username);

int load_all_users(FILE * file, user_data * user_data);


#endif