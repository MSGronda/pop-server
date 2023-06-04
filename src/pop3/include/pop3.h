#ifndef POP3_H
#define POP3_H

typedef struct client_connection_data client_connection_data;

#include "../../utils/include/selector.h"
#include "../../utils/include/buffer.h"
#include "../../include/common.h"
#include "../../utils/include/stm.h"
#include "../../parser/include/parser.h"
#include "./socket_io_actions.h"
#include <stdlib.h>
#include <string.h>


#define BUFFER_SIZE 4096

// = = = = = MAQUINA DE ESTADOS = = = = = 

typedef enum {
    SOCKET_IO_WRITE,
    SOCKET_IO_READ
}stm_io_state;


// = = = = = ESTADO DE CLIENTE POP3 = = = = = 

typedef enum {
    AUTH_INI,               // CAPA, USER
    AUTH_PASSWORD,          // PASS
    TRANSACTION,            // LIST, RETR, DELE...
    UPDATE,                 // ---
}pop3_state;


// EXP: Contiene toda informacion relevante a un cliente
typedef struct client_connection_data{
    int client_fd;                              // fd del socket del client
    struct sockaddr_storage client_address;     // informacion de la direccion del cliente

    buffer read_buffer;                       
    buffer write_buffer;

    uint8_t read_addr[BUFFER_SIZE];
    uint8_t write_addr[BUFFER_SIZE];
    
    struct state_machine stm;                   // maquina de entrada y salida del cliente

    pop3_state state;                           // estado del cliente de pop3: AUTHENTICATION, TRANSACTION, UPDATE

    input_parser command_parser;                // parser de comandos pop3

    client_connection_data * next;              // proximo cliente en la lista de clientes
}client_connection_data;


void pop3_passive_handler(struct selector_key *key);

#endif
