#include "./include/pop3.h"

// = = = = = MACROS Y CONSTANTES = = = = = 

#define BUFFER_SIZE 4096

// = = = = = MAQUINA DE ESTADOS DE E\S = = = = = 

static const struct state_definition client_io_state_actions[] = {
    {
        .state = REQ_READ_STATE,
        .on_read_ready = &pop3_read_action,
    },
    {
        .state = REQ_WRITE_STATE,
        .on_write_ready = &pop3_write_action,
    },
};

// = = = = = SETUP ESTADO DE CLIENTE = = = = = 

// EXP: Contiene el primer nodo en la lista de conexiones 
static client_connection_data * connection_pool = NULL; 

client_connection_data * find_last_connection() {
    if(connection_pool == NULL) {
        return NULL;
    }

    client_connection_data * node = connection_pool;
    while(node->next != NULL) {
        node = node->next;
    }

    return node;
}

client_connection_data * setup_new_connection(int client_fd, struct sockaddr_storage client_address) {
    client_connection_data * new_connection = malloc(sizeof(client_connection_data));

    // supuestamente malloc no deberia retornar NULL nunca pero bueno
    if(new_connection == NULL) {
        return NULL;
    }    

    if(connection_pool == NULL) {
        connection_pool = new_connection;
    }
    else{
        client_connection_data * last = find_last_connection();
        last->next = new_connection;
    }

    memset(new_connection, 0, sizeof(client_connection_data));

    new_connection->client_fd = client_fd;
    new_connection->client_address = client_address;


    // = = = = = INICIALIZO BUFFERS = = = = = 

    uint8_t * read_buffer = malloc(BUFFER_SIZE);
    uint8_t * write_buffer = malloc(BUFFER_SIZE);

    // supuestamente malloc no deberia retornar NULL nunca pero bueno
    if(read_buffer == NULL || write_buffer == NULL) {        // TODO: check this
        return NULL;
    }

    buffer_init(&new_connection->read_buffer,BUFFER_SIZE, read_buffer);
    buffer_init(&new_connection->write_buffer,BUFFER_SIZE, write_buffer);
    

    // = = = = = INICIALIZO ESTADO DE CLIENTE = = = = = 

    new_connection->state = AUTH_INI;

    // = = = = = INICIALIZO MAQUINA DE ESTADOS DE E/S = = = = = 

    // Seteo de la maquina de estados
    new_connection->stm.initial = REQ_READ_STATE;
    new_connection->stm.max_state = REQ_WRITE_STATE;
    new_connection->stm.states = client_io_state_actions;

    // Inicialización de la máquina de estados
    stm_init(&new_connection->stm);

    return new_connection;
}

// = = = = = HANDLERS = = = = = 

void pop3_read_handler(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    stm_handler_read(stm, key);                             // TODO: return value (?)
}
void pop3_write_handler(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    stm_handler_write(stm, key);
}
void pop3_block_handler(struct selector_key *key) {
    printf("BLOCK");                                        // TODO: make 
}

void pop3_close_handler(struct selector_key *key) {     // TODO: liberar recursos en este llamado.
    printf("CLOSE");
}



static const struct fd_handler pop3_handlers ={
    .handle_read = &pop3_read_handler,
    .handle_write = &pop3_write_handler,
    .handle_close = &pop3_close_handler,
    .handle_block = &pop3_block_handler,
};

void pop3_passive_handler(struct selector_key *key) {
    char * error_msg;
    printf("A connection has been recieved\n");

    struct sockaddr_storage client_address;
    socklen_t client_address_len = sizeof(client_address);

    // = = = = = CONFIGURACION DEL SOCKET DE CLIENTE = = = = = =

    // EXP: saco el cliente del queue y genero un socket activo 
    const int client_fd = accept(key->fd, (struct sockaddr *)&client_address, &client_address_len);

    if(client_fd == -1) {
        ERROR_CATCH("Error accepting client connection", finally)
    }
    
    // EXP: seteo como no bloqueante
    if(selector_fd_set_nio(client_fd) == -1) {
        ERROR_CATCH("Error setting client socket as non-blocking", finally)
    }

    // EXP: hago el setup de toda la informacion del cliente: address, fd, buffers, maquina de estados, etc
    client_connection_data * new_client = setup_new_connection(client_fd, client_address);
    
    if(new_client == NULL) {
        ERROR_CATCH("Error generating client connection data", finally)
    }

    // EXP: configuro todos los handlers para los distintos casos: read, write, close, block
    // EXP: lo registramos con el read dado que queremos que se "active" cuando el cliente manda algo
    if(selector_register(key->s, client_fd, &pop3_handlers, OP_READ, new_client) != SELECTOR_SUCCESS) {
        ERROR_CATCH("Error registering client socket to select", finally)
    }
    
    // TODO: setup de las stats de conexion o algo asi
    return;

finally:
    fprintf(stderr, "%s\n", error_msg);
}
