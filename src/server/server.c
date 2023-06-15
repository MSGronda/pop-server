#include "./include/server.h"


struct pop3_server_state server_state;

// = = = = = = = = =  SERVER STATE  = = = = = = = = =

bool initialize_server_state(){
    memset(&server_state, 0, sizeof(struct pop3_server_state));

    server_state.running = true;

    server_state.users = malloc(MAX_USERS * sizeof(users_data));
    if(server_state.users == NULL){
        return false;
    }

    return true;
}

void destroy_server_state(){
    server_state.running = false;

    free(server_state.users);
}

struct pop3_server_state * get_server_state() {
    return &server_state;
}

void init_server_metrics(server_metrics * metrics){
    metrics->total_connections = 0;
    metrics->current_connections = 0;
    metrics->bytes_sent = 0;
    metrics->bytes_recieved = 0;
}

void update_connections(server_metrics * metrics){
    
}



