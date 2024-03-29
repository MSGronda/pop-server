#include <string.h>
#include <stdlib.h>

#include "../pop3/include/pop3.h"
#include "../pop3/include/users.h"

#include "./include/m3.h"
#include "./include/server.h"


struct pop3_server_state server_state;

// = = = = = = = = =  SERVER METRICS  = = = = = = = = =

void init_server_metrics() {
    server_state.metrics.total_connections = 0;
    server_state.metrics.current_connections = 0;
    server_state.metrics.bytes_sent = 0;
    server_state.metrics.bytes_recieved = 0;
}

void metrics_add_connection() {
    server_state.metrics.current_connections++;
    server_state.metrics.total_connections++;
}

void metrics_remove_connection() {
    server_state.metrics.current_connections--;
}

void metrics_sent_bytes(uint32_t sent) {
    server_state.metrics.bytes_sent += sent;
}

void metrics_recieved_bytes(uint32_t recieved) {
    server_state.metrics.bytes_recieved += recieved;
}


// = = = = = = = = =  SERVER STATE  = = = = = = = = =

bool initialize_server_state() {
    memset(&server_state, 0, sizeof(struct pop3_server_state));

    server_state.running = true;
    server_state.port = STANDARD_PORT;
    server_state.mng_port = M3_STANDARD_PORT;

    char * token = getenv("M3_AUTH_TOKEN");
    server_state.mng_auth_token = token == NULL ? DEFAULT_AUTH_TOKEN : atoi(token); 

    server_state.users = malloc(MAX_USERS * sizeof(users_data));
    if(server_state.users == NULL) {
        return false;
    }

    init_server_metrics();

    return true;
}

void free_server_resources() {
    server_state.running = false;

    destroy_all_connections();

    free_users();
    free(server_state.users);

    log(DEBUG, "%s", "All server resources successfully freed")
}

struct pop3_server_state * get_server_state() {
    return &server_state;
}




