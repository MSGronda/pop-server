#ifndef POP3_H
#define POP3_H


#include "../../utils/include/selector.h"
#include "../../utils/include/buffer.h"
#include <stdlib.h>


void pop3_passive_handler(struct selector_key *key);

#endif
