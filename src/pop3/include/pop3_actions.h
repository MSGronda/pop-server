#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include "users.h"
#include "./pop3.h"
#include "../../parser/include/parser.h"
#include "./mails.h"
#include <string.h>

typedef struct client_connection_data client_connection_data;

// EXP: puntero a funcion que ejecuta la accion
typedef int (*pop3_action)(client_connection_data * );


void pop3_action_handler(client_connection_data * client_data, command_state cmd_state);

#endif