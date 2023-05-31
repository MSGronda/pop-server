#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include "./pop3.h"

unsigned int pop3_read_action(struct selector_key *key);
unsigned int pop3_write_action(struct selector_key *key);

#endif
