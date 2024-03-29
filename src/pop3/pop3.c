// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <string.h>
#include <stdlib.h>

#include "./include/users.h"
#include "./include/socket_io_actions.h"

#include "../utils/include/buffer.h"
#include "../parser/include/parser.h"
#include "../server/include/server.h"

#include "./include/pop3.h"
#include "./include/pop3_actions.h"

// = = = = = FUNCTIONS = = = = = 
void pop3_close_handler(struct selector_key *key);
void pop3_read_handler(struct selector_key *key);
void pop3_write_handler(struct selector_key *key);


// = = = = = SETUP ESTADO DE CLIENTE = = = = = 

// EXP: Contiene el primer nodo en la lista de conexiones 
static client_connection_data * connection_pool = NULL; 

client_connection_data * find_previous_connection(client_connection_data * client_data) {
    if(connection_pool == NULL) {
        return NULL;
    }
    if(connection_pool == client_data) {
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
        if(last != NULL) {
            last->next = new_connection;
        }
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
    new_connection->command.command_num = CMD_NOT_RECOGNIZED;

    log(DEBUG, "Initialized client data for client with fd: %d", new_connection->client_fd)

    return new_connection;
}

void destroy_connection_info(client_connection_data * client_data) {

    // EXP: desactivo flag para indicar que usuario no esta logueado
    if(client_data->username != NULL) {
        logout_user(client_data->username);
    }

    // EXP: elimino de la lista y libero recursos
    client_connection_data * previous = find_previous_connection(client_data);

    if(previous == NULL) {
        connection_pool = NULL;
    }
    else if(previous == client_data) {
        connection_pool = client_data->next;
    }
    else {
        previous->next = client_data->next;
    }
    if(client_data->command_parser.current_command.argument != NULL) {
        free(client_data->command_parser.current_command.argument);
    }
    if(client_data->username != NULL) {
        free(client_data->username);
    }
    if(client_data->mail_info != NULL) {
        free(client_data->mail_info);
    }
    free(client_data);
}

void destroy_all_connections() {
    while(connection_pool != NULL) {
        if(connection_pool->client_fd != -1) {
            close(connection_pool->client_fd);
        }
        free_mail_info_no_key(connection_pool);
        destroy_connection_info(connection_pool);
    }
    log(DEBUG, "%s", "Destroyed all connections")
}


bool pop3_interpret_command(client_connection_data * client_data) {

    // EXP: hacemos la escritura al buffer  y luego la lecutra (en el parser)
    // EXP: en 2 pasos pues puede ya haber (de una transmision anterior) en el buffer
    bool finished = 0;
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

    bool complete_command = pop3_interpret_command(client_data);

    if(complete_command) {
        // EXP: ejecuto el comando y me paso para escribir respuesta
        pop3_action_handler(key);
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

    // EXP: usamos else if para obligar que primero escriba al socket antes de continuar

    // EXP: no termino de ejecutarse el comando (por falta de espacio en el buffer), continuo    
    if(!client_data->command.finished) {
        pop3_continue_action(key);
    }
    // EXP: todavia hay comandos en el buffer de lectura (por pipelining), hay que consumir y ejecutar
    else if(buffer_can_read(&client_data->read_buffer)) {
        bool complete_command = pop3_interpret_command(client_data);

        if(complete_command) {
            pop3_action_handler(key);
        }
        else{
            // EXP: el comando que lei esta incompleto, espero a que el usuario mande algo
            if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                log(ERROR, "%s", "Error setting interest from OP_WRITE to OP_READ")
                return;
            }
        }
    }
    // EXP: puede mandar todo. ahora tengo que esperar hasta que el usuario mande algo
    else if(!buffer_can_read(&client_data->write_buffer)) {
        if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
            log(ERROR, "%s", "Error setting interest from OP_WRITE to OP_READ")
            return;
        }
    }
    
    if(client_data->state == CLIENT_FINISHED) {
        pop3_close_handler(key);
    }
}

void pop3_close_handler(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: debemos hacer esto porque selector_unregister_fd llama al close handler cuando termina
    if(!client_data->active) {
        return;
    }
    client_data->active = 0;
    client_data->command.finished = true;

    log(DEBUG, "Closing connection with fd: %d", client_data->client_fd)

    if(client_data->client_fd != -1) {
        selector_unregister_fd(key->s, client_data->client_fd);
        close(client_data->client_fd);
    }

    free_mail_info(key);

    destroy_connection_info(client_data);

    metrics_remove_connection();
}

static const struct fd_handler pop3_handlers ={
    .handle_read = &pop3_read_handler,
    .handle_write = &pop3_write_handler,
    .handle_close = &pop3_close_handler,
    .handle_block = NULL
};

void pop3_passive_handler(struct selector_key *key) {
    char * error_msg;
    int client_fd;
    client_connection_data * new_client = NULL;

    struct sockaddr_storage client_address;
    socklen_t client_address_len = sizeof(client_address);

    // = = = = = CONFIGURACION DEL SOCKET DE CLIENTE = = = = = =

    // EXP: saco el cliente del queue y genero un socket activo OP_WRITE
    client_fd = accept(key->fd, (struct sockaddr *)&client_address, &client_address_len);

    if(client_fd == -1) {
        ERROR_CATCH("Error accepting client connection", finally)
    }
    
    // EXP: seteo como no bloqueante
    if(selector_fd_set_nio(client_fd) == -1) {
        ERROR_CATCH("Error setting client socket as non-blocking", finally)
    }

    // EXP: hago el setup de toda la informacion del cliente: address, fd, buffers, maquina de estados, etc
    new_client = setup_new_connection(client_fd, client_address);

    if(new_client == NULL) {
        ERROR_CATCH("Error generating client connection data", finally)
    }

    // EXP: configuro todos los handlers para los distintos casos: read, write, close, block
    // EXP: lo registramos como OP_WRITE ya que queremos primero mandar el "+OK server ready" antes de recibir comandos
    if(selector_register(key->s, client_fd, &pop3_handlers, OP_WRITE, new_client) != SELECTOR_SUCCESS) {
        ERROR_CATCH("Error registering client socket to select", finally)
    }
    
    log(DEBUG, "New connection recieved with fd: %d", client_fd)

    metrics_add_connection();

    return;

finally:
    log(ERROR, "%s", error_msg);

    if(client_fd != -1) {
        close(client_fd);
    }
    if(new_client != NULL) {
        destroy_connection_info(new_client);
    }
}
