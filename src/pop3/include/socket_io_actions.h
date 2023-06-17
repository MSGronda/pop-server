#ifndef SOCKET_IO_ACTIONS_H
#define SOCKET_IO_ACTIONS_H

#include "../../utils/include/selector.h"

// = = = = = ESTADO DE SOCKET = = = = = 

typedef enum {
    SOCKET_WRITE_OK,
    SOCKET_READ_OK,
    SOCKET_DONE,
    SOCKET_ERROR
}socket_state;

unsigned int socket_read(struct selector_key *key);
unsigned int socket_write(struct selector_key *key);

#endif
