#ifndef POP3_ACTIONS_H
#define POP3_ACTIONS_H

#include "./pop3.h"

void greeting_arrival(const unsigned state, struct selector_key *key);
unsigned int greeting_write(struct selector_key *key);

unsigned int command_read(struct selector_key *key);
unsigned int command_write(struct selector_key *key);

#endif
