// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "include/users.h"

bool add_user(char *s) {
    struct pop3_server_state * server_state = get_server_state();

    if(server_state->amount_users >= MAX_USERS){
        return false;
    }

    users_data * user = &server_state->users[server_state->amount_users];

    server_state->amount_users++;

    char *p = strchr(s, ':');
    if(p == NULL) {
        // TODO: change
        fprintf(stderr, "password not found\n");
        exit(1);
    } else {
        *p = 0;
        p++;
        user->name = s;
        user->pass = p;
        user->sessionActive = false;
    }
    return true;
}

int find_user(const char * username) {
    struct pop3_server_state * server_state = get_server_state();

    unsigned i;
    for (i = 0; i < server_state->amount_users; i++) {
        if (strcmp(server_state->users[i].name, username) == 0) {
            return i;
        }
    }
    return -1;
}


user_status login_user(const char * username, const char * password) {
    struct pop3_server_state * server_state = get_server_state();
    if(server_state->amount_users == 0) {
        return GENERAL_ERROR;
    }

    int user_index = find_user(username);
    if (user_index < 0)
        return USER_ERROR;
    int pass = strcmp(server_state->users[user_index].pass, password);
    if(pass != 0 || server_state->users[user_index].sessionActive == true )
        return USER_ERROR;

    server_state->users[user_index].sessionActive = true;
    return USER_OK;
}

user_status logout_user(const char * username){
    struct pop3_server_state * server_state = get_server_state();
    //No deberia entrar a ningun caso del if pero se deja por claridad
    if(server_state->amount_users == 0) {
        return GENERAL_ERROR;
    }
    int user_index = find_user(username);
    if (user_index < 0)
        return USER_ERROR;
    if( server_state->users[user_index].sessionActive == false )
        return USER_ERROR;
    server_state->users[user_index].sessionActive = false;
    return USER_OK;
}


