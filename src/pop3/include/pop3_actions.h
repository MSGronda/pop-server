#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include "users.h"
#include "./pop3.h"
#include "../../parser/include/parser.h"
#include "./mails.h"
#include <string.h>

typedef struct client_connection_data client_connection_data;

// EXP: puntero a funcion que ejecuta la accion
typedef int (*pop3_action)(struct selector_key *key );


void pop3_action_handler(struct selector_key *key);
void pop3_continue_action(struct selector_key *key);

#endif