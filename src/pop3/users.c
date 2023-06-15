// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "include/users.h"



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


