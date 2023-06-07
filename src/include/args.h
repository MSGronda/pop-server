#ifndef ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdbool.h>

#define MAX_USERS 10

typedef struct {
    char *name;
    char *pass;
}users_data;

struct pop3_server_state {
    unsigned short port;
    char * folder_address;
    unsigned short mng_port;

    users_data users[MAX_USERS];
    size_t amount_users;
};

void parse_args(int argc, char * argv[], struct pop3_server_state * pop3_server_state);

#endif