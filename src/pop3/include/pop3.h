#ifndef POP3_H
#define POP3_H

#include <string.h>
#include <stdlib.h>

#include "./pop3_structures.h"
#include "./socket_io_actions.h"

#include "../../include/common.h"
#include "../../utils/include/selector.h"
#include "../../utils/include/buffer.h"
#include "../../parser/include/parser.h"
#include "../../server/include/server.h"



void pop3_passive_handler(struct selector_key *key);

#endif
