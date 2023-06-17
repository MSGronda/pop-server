#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdint.h>

// = = = = = METRICAS DEL SERVIDOR POP3 = = = = = 

typedef struct server_metrics{
    uint32_t total_connections;
    uint32_t current_connections;
    uint32_t bytes_sent;
    uint32_t bytes_recieved; 
}server_metrics;

// = = = = = ESTADO DEL SERVIDOR POP3 = = = = = 

#define MAX_USERS 20
#define STANDARD_PORT 44443
#define M3_STANDARD_PORT 55552

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
void metrics_sent_bytes(uint32_t sent);
void metrics_recieved_bytes(uint32_t recieved);

#endif