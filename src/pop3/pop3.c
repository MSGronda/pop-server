#include "./include/pop3.h"

// = = = = = FUNCTIONS = = = = = 
void pop3_close_handler(struct selector_key *key);
void pop3_block_handler(struct selector_key *key);
void pop3_read_handler(struct selector_key *key);
void pop3_write_handler(struct selector_key *key);

// = = = = = MAQUINA DE ESTADOS DE E\S = = = = = 

static const struct state_definition client_state_actions[] = {
    {
        .state = SOCKET_IO_WRITE,
        .on_write_ready = &socket_write,
    },
    {
        .state = SOCKET_IO_READ,
        .on_read_ready = &socket_read,
    },
    {
        .state = SOCKET_DONE,
        .on_arrival = &socket_done,
    },
    {
        .state = SOCKET_ERROR,
        .on_arrival = &socket_error,
    },
};

// = = = = = SETUP ESTADO DE CLIENTE = = = = = 

// EXP: Contiene el primer nodo en la lista de conexiones 
static client_connection_data * connection_pool = NULL; 

client_connection_data * find_previous_connection(client_connection_data * client_data) {
    if(connection_pool == NULL) {
        return NULL;
    }
    if(connection_pool == client_data){
        return client_data;
    }

    client_connection_data * node = connection_pool;
    while(node->next != client_data) {                    // EXP: comparamos las direcciones de memoria directamente
        node = node->next;
    }

    return node;
}

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

    buffer_init(&new_connection->read_buffer, BUFFER_SIZE, new_connection->read_addr);
    buffer_init(&new_connection->write_buffer, BUFFER_SIZE, new_connection->write_addr);

    //  = = = = = MENSAJE INICIAL = = = = = 
    // EXP: copio el HELLO ahora y saco logica de greeting y todo eso de las actions
    char * hello_msg = "+OK pop3-server ready\n";
    size_t hello_len = strlen(hello_msg);

    // WARNING: asumo que hay suficiente espacio en el buffer para escribir todo el mensaje
    size_t max_size;
    uint8_t * p = buffer_write_ptr(&new_connection->write_buffer, &max_size);
    memcpy(p, hello_msg, hello_len);
    buffer_write_adv(&new_connection->write_buffer, strlen(hello_msg));

    new_connection->write_finished = 1;
    new_connection->msg_pos = 0;

    // = = = = = INICIALIZO DE ESTADO DE POP3 = = = = = 

    new_connection->state = AUTH_INI;
    new_connection->active = 1;

    // = = = = = INICIALIZO MAQUINA DE ESTADOS DE E/S = = = = = 

    // Seteo de la maquina de estados
    new_connection->stm.initial = SOCKET_IO_WRITE;
    new_connection->stm.max_state = SOCKET_ERROR;
    new_connection->stm.states = client_state_actions;

    // Inicialización de la máquina de estados
    stm_init(&new_connection->stm);

    // = = = = = INICIALIZO DE PARSER DE COMANDOS = = = = = 
    
    parser_init(&new_connection->command_parser);

    return new_connection;
}

// = = = = = HANDLERS = = = = = 

void pop3_read_handler(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    unsigned int io_state = stm_handler_read(stm, key);
    if(io_state == SOCKET_DONE || io_state == SOCKET_ERROR) {
        pop3_close_handler(key);
    }
}
void pop3_write_handler(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    unsigned int io_state = stm_handler_write(stm, key);            //TODO: return value (?)
}
void pop3_block_handler(struct selector_key *key) {
    printf("BLOCK");                                        // TODO: make 
}

void pop3_close_handler(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: debemos hacer esto porque selector_unregister_fd llama al close handler cuando termina
    if(!client_data->active){
        return;
    }
    client_data->active = 0;

    if(client_data->client_fd != -1) {
        selector_unregister_fd(key->s, client_data->client_fd);
        close(client_data->client_fd);
    }

    // EXP: elimino de la lista y libero recursos
    client_connection_data * previous = find_previous_connection(client_data);

    if(previous == NULL || previous == client_data) {
        connection_pool = NULL;
    }
    else {
        previous->next = client_data->next;
    }
    free(client_data);
}



static const struct fd_handler pop3_handlers ={
    .handle_read = &pop3_read_handler,
    .handle_write = &pop3_write_handler,
    .handle_close = &pop3_close_handler,
    .handle_block = &pop3_block_handler,
};

void pop3_passive_handler(struct selector_key *key) {
    char * error_msg;
    printf("A connection has been recieved\n");     // TODO: REMOVE

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
    // EXP: lo registramos como OP_WRITE ya que queremos primero mandar el "+OK server ready" antes de recibir comandos
    if(selector_register(key->s, client_fd, &pop3_handlers, OP_WRITE, new_client) != SELECTOR_SUCCESS) {
        ERROR_CATCH("Error registering client socket to select", finally)
    }
    
    // TODO: setup de las stats de conexion o algo asi
    return;

finally:
    fprintf(stderr, "%s\n", error_msg);
}
