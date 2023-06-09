#ifndef SOCKET_IO_ACTIONS_H
#define SOCKET_IO_ACTIONS_H

#include "./pop3.h"
#include "./pop3_actions.h"
#include "../../parser/include/parser.h"
#include "../../utils/include/selector.h"

unsigned int socket_read(struct selector_key *key);
unsigned int socket_write(struct selector_key *key);

#endif
