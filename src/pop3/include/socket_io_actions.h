#ifndef SOCKET_IO_ACTIONS_H
#define SOCKET_IO_ACTIONS_H

#include "./pop3.h"
#include "./pop3_actions.h"
#include "../../parser/include/parser.h"


unsigned int socket_read(struct selector_key *key);
unsigned int socket_write(struct selector_key *key);
void socket_done(const unsigned state, struct selector_key *key);
void socket_error(const unsigned state, struct selector_key *key);

#endif
