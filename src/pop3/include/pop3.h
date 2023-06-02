#ifndef POP3_H
#define POP3_H


#include "../../utils/include/selector.h"
#include "../../utils/include/buffer.h"
#include "../../include/common.h"
#include "../../utils/include/stm.h"
#include "./socket_io_actions.h"
#include <stdlib.h>


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

typedef struct client_connection_data client_connection_data;

// EXP: Contiene toda informacion relevante a un cliente
typedef struct client_connection_data{
    int client_fd;                              // fd del socket del client
    struct sockaddr_storage client_address;     // informacion de la direccion del cliente

    buffer read_buffer;                       
    buffer write_buffer;

    struct state_machine stm;                   // maquina de estados para este cliente

    pop3_state state;                           // estado del cliente de pop3: AUTHENTICATION, TRANSACTION, UPDATE

    client_connection_data * next;              // proximo cliente en la lista de clientes
}client_connection_data;


void pop3_passive_handler(struct selector_key *key);

#endif
