#ifndef SERVER_H
#define SERVER_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// = = = = = METRICAS DEL SERVIDOR POP3 = = = = = 

typedef struct server_metrics{
    size_t total_connections;
    size_t current_connections;
    size_t bytes_sent;
    size_t bytes_recieved; 
}server_metrics;

// = = = = = ESTADO DEL SERVIDOR POP3 = = = = = 

#define MAX_USERS 20
#define STANDARD_PORT 25565

typedef struct {
    char * name;
    char * pass;
    char * maildir_path;
    bool sessionActive;
}users_data;

struct pop3_server_state {
    bool running;
    unsigned short port;
    char * folder_address;
    unsigned short mng_port;

    users_data * users;
    size_t amount_users;

    server_metrics metrics;
};


bool initialize_server_state();
void destroy_server_state();
struct pop3_server_state * get_server_state();

void metrics_add_connection();
void metrics_remove_connection();
void metrics_sent_bytes(size_t sent);
void metrics_recieved_bytes(size_t recieved);

#endif