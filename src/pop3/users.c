#include "include/users.h"


// all users stored in system
static users_data * users;
static int users_count; 



void load_users( users_data users_array[], size_t amount_users) {
    users = users_array;
    users_count = amount_users;
}

void skip_line(FILE * file) {
    int c;
    while ((c = getc(file)) != '\n') {
        //EOF or error
        if (c < 0) {
            return;
        }
    }
}

int find_user(const char * username) {
    int i;
    for (i = 0; i < users_count; i++) {
        if (strcmp(users[i].name, username) == 0) {
            return i;
        }
    }
    return -1;
}


user_status login_user(const char * username, const char * password) {
    if(users_count == 0) {
        return GENERAL_ERROR;
    }

    int user_index = find_user(username);
    if (user_index < 0)
        return USER_ERROR;
    int pass = strcmp(users[user_index].pass, password);
    if(pass != 0)
        return USER_ERROR;

    return USER_OK;
}   


int finish_users() {
    free(users);
    return USER_OK;
}
