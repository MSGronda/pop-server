#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include "users.h"
#include "./pop3.h"
#include "../../parser/include/parser.h"
#include <string.h>

void pop3_action_handler(client_connection_data * client_data, command_state cmd_state);

#endif