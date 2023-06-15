#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include <string.h>

#include "./pop3_structures.h"
#include "./users.h"
#include "./mails.h"

#include "../../parser/include/parser.h"
#include "../../server/include/server.h"

void pop3_action_handler(struct selector_key *key);
void pop3_continue_action(struct selector_key *key);

#endif