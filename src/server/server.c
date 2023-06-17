#include <string.h>
#include <stdlib.h>

#include "./include/server.h"

struct pop3_server_state server_state;

// = = = = = = = = =  SERVER METRICS  = = = = = = = = =

void init_server_metrics(){
    server_state.metrics.total_connections = 0;
    server_state.metrics.current_connections = 0;
    server_state.metrics.bytes_sent = 0;
    server_state.metrics.bytes_recieved = 0;
}

void metrics_add_connection(){
    server_state.metrics.current_connections++;
    server_state.metrics.total_connections++;
}

void metrics_remove_connection(){
    server_state.metrics.current_connections--;
}

void metrics_sent_bytes(uint32_t sent){
    server_state.metrics.bytes_sent += sent;
}

void metrics_recieved_bytes(uint32_t recieved){
    server_state.metrics.bytes_recieved += recieved;
}


// = = = = = = = = =  SERVER STATE  = = = = = = = = =

bool initialize_server_state(){
    memset(&server_state, 0, sizeof(struct pop3_server_state));

    server_state.running = true;
    server_state.port = STANDARD_PORT;
    server_state.mng_port = M3_STANDARD_PORT;

    server_state.users = malloc(MAX_USERS * sizeof(users_data));
    if(server_state.users == NULL){
        return false;
    }

    init_server_metrics();

    return true;
}

void destroy_server_state(){
    server_state.running = false;

    free(server_state.users);
}

struct pop3_server_state * get_server_state() {
    return &server_state;
}




