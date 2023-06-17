#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include "./pop3.h"
#include "../../utils/include/selector.h"

void pop3_action_handler(struct selector_key *key);
void pop3_continue_action(struct selector_key *key);

#endif