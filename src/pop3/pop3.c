#include "./include/pop3.h"

// = = = = = FUNCTIONS = = = = = 
void pop3_close_handler(struct selector_key *key);
void pop3_block_handler(struct selector_key *key);
void pop3_read_handler(struct selector_key *key);
void pop3_write_handler(struct selector_key *key);


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
    char * hello_msg = "+OK pop3-server ready\r\n";
    size_t hello_len = strlen(hello_msg);

    buffer_write_n(&new_connection->write_buffer, hello_msg, hello_len);

    // = = = = = INICIALIZO DE ESTADO DE POP3 = = = = = 

    new_connection->state = AUTH_INI;
    new_connection->active = 1;

    // = = = = = INICIALIZO DE PARSER DE COMANDOS = = = = = 
    
    parser_init(&new_connection->command_parser);

    // = = = = = INICIALIZO DE COMANDO ACTUAL = = = = = 

    new_connection->command.finished = 1;
    new_connection->command.bytes_written = 0;
    new_connection->command.action = NULL;

    return new_connection;
}

bool pop3_interpret_command(struct selector_key *key){
    client_connection_data * client_data = ATTACHMENT(key);
    // EXP: hacemos la escritura al buffer  y luego la lecutra (en el parser)
    // EXP: en 2 pasos pues puede ya haber (de una transmision anterior) en el buffer
    bool finished = 0;          // TODO: check esto porque si o si hay que inicializarlo en 0
    size_t consumed = 0;
    parser_consume(&client_data->command_parser, &client_data->read_buffer, &finished, &consumed);

    // EXP: el comando esta incompleto, debemos "esperar" hasta que llegue mas informacion
    return finished;
}

// = = = = = HANDLERS = = = = = 

void pop3_read_handler(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: recibo del cliente
    unsigned int io_state = socket_read(key);

    if(io_state == SOCKET_DONE || io_state == SOCKET_ERROR) {
        pop3_close_handler(key);
        return;
    }

    bool complete_command = pop3_interpret_command(key);

    if(complete_command){
        // EXP: ejecuto el comando y me paso para escribir respuesta
        pop3_action_handler(client_data, client_data->command_parser.state);
        selector_set_interest_key(key, OP_WRITE);
    }   
}

void pop3_write_handler(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: envio al cliente lo que esta en el buffer de salida
    unsigned int io_state = socket_write(key);

    if(io_state == SOCKET_ERROR) {
        pop3_close_handler(key);
        return;
    }

    // EXP: hay 3 casos por los cuales nos queremos mantener en escritura:
    // 1) no se pudo mandar todo al cliente (la accion es multiline)
    // 2) todavia hay comandos para le`er y ejecutar (por pipelining)
    // 3) no se le pudo mandar todo al cliente (en cualquier caso: single line, multiline, pipelining, etc)

    // EXP: usamos else if para obligar que primero mande la respuesta al usaurio siempre antes de continuar

    // EXP: no termino de ejecutarse el comando (por falta de espacio en el buffer), continuo    
    if(!client_data->command.finished){
        client_data->command.finished = client_data->command.action(client_data);

        // EXP: reseteo comando 
        if(client_data->command.finished){
            client_data->command.bytes_written = 0;
            client_data->command.action = NULL;
        }
    }
    // EXP: todavia hay comandos en el buffer de lectura (por pipelining), hay que consumir y ejecutar
    else if(buffer_can_read(&client_data->read_buffer)){
        bool complete_command = pop3_interpret_command(key);

        if(complete_command){
            pop3_action_handler(client_data, client_data->command_parser.state);
        }
        else{
            // EXP: el comando que lei esta incompleto, espero a que el usuario mande algo
            selector_set_interest_key(key, OP_READ);
        }
    }
    // EXP: puede mandar todo. ahora tengo que esperar hasta que el usuario mande algo
    else if(!buffer_can_read(&client_data->write_buffer)){
        selector_set_interest_key(key, OP_READ);
    } 
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

    // EXP: desactivo flag para indicar que usuario no esta logueado
    if( client_data->username != NULL){
        logout_user(client_data->username);
    }

    // EXP: elimino de la lista y libero recursos
    client_connection_data * previous = find_previous_connection(client_data);

    if(previous == NULL) {
        connection_pool = NULL;
    }
    else if (previous == client_data){
        connection_pool = client_data->next;
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
