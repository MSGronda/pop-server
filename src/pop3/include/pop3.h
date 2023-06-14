#ifndef POP3_H
#define POP3_H

#include <stdlib.h>
#include "../../utils/include/selector.h"

typedef struct client_connection_data client_connection_data;
typedef int (*pop3_action)(struct selector_key *key );

#include "./mails.h"
#include "./socket_io_actions.h"
#include "pop3_actions.h"

#include "../../utils/include/buffer.h"
#include "../../include/common.h"
#include "../../parser/include/parser.h"
#include <string.h>


#define BUFFER_SIZE 400

// = = = = = COMMAND ACTUAL = = = = = 

// EXP: sirve para aquellos comandos que son multi linea y que quizas 
// EXP tenga que continuar la ejecucion luego de ser pausada (por falta de buffer)

typedef struct running_command{
    bool finished;
    size_t bytes_written;
    command_type command_num;
}running_command;

// = = = = = MAQUINA DE ESTADOS = = = = = 

typedef enum {
    SOCKET_WRITE_OK,
    SOCKET_READ_OK,
    SOCKET_DONE,
    SOCKET_ERROR
}socket_state;


// = = = = = ESTADO DE CLIENTE POP3 = = = = = 

typedef enum {
    AUTH_INI,               // CAPA, USER, QUIT
    AUTH_PASSWORD,          // PASS, QUIT
    TRANSACTION,            // LIST, RETR, DELE...
    CLIENT_FINISHED         // hay que cerrar la conexion 
}pop3_state;


// EXP: Contiene toda informacion relevante a un cliente
typedef struct client_connection_data{
    int client_fd;                              // fd del socket del client
    struct sockaddr_storage client_address;     // informacion de la direccion del cliente

    buffer read_buffer;                       
    buffer write_buffer;

    uint8_t read_addr[BUFFER_SIZE];
    uint8_t write_addr[BUFFER_SIZE];

    pop3_state state;                           // estado del cliente de pop3: AUTHENTICATION, TRANSACTION, UPDATE

    input_parser command_parser;                // parser de comandos pop3
    
    running_command command;                    // el comando que actualmente se esta corriendo

    int active;                                 // designa si un socket sigue activo o ha sido cerrado (por error u otra razon)

    char * username;                            // client username

    user_mail_info mail_info;                   // informacion sobre los mails en la casilla del usuario

    client_connection_data * next;              // proximo cliente en la lista de clientes
}client_connection_data;


void pop3_passive_handler(struct selector_key *key);

#endif
