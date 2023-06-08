#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include "users.h"
#include "./pop3.h"
#include "../../parser/include/parser.h"
#include "./mails.h"
#include <string.h>

// = = = = = COMMAND ACTUAL = = = = = 

// EXP: sirve para aquellos comandos que son multi linea y que quizas 
// EXP tenga que continuar la ejecucion luego de ser pausada (por falta de buffer)

typedef struct running_command{
    short finished;
    size_t bytes_written;
    command_type command;
}running_command;


void pop3_action_handler(client_connection_data * client_data, command_state cmd_state);

#endif