#ifndef POP3_H
#define POP3_H

#include <stdlib.h>
#include <sys/socket.h>

#include "./mails.h"

#include "../../include/common.h"
#include "../../utils/include/selector.h"
#include "../../parser/include/parser.h"

// = = = = = ESTADO DE CLIENTE POP3 = = = = = 

#define BUFFER_SIZE 8192

typedef enum {
    AUTH_INI,               // CAPA, USER, QUIT
    AUTH_PASSWORD,          // PASS, QUIT
    TRANSACTION,            // LIST, RETR, DELE...
    CLIENT_FINISHED         // hay que cerrar la conexion 
}pop3_state;

// EXP: sirve para aquellos comandos que son multi linea y que quizas 
// EXP tenga que continuar la ejecucion luego de ser pausada (por falta de buffer)
typedef struct running_command{
    bool finished;
    size_t bytes_written;
    command_type command_num;
}running_command;

typedef struct client_connection_data client_connection_data;
typedef struct user_mail_info user_mail_info;


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

    user_mail_info * mail_info;                   // informacion sobre los mails en la casilla del usuario

    client_connection_data * next;              // proximo cliente en la lista de clientes
}client_connection_data;


void pop3_passive_handler(struct selector_key *key);
void destroy_all_connections();

#endif
