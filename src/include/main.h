#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/signal.h>

#include "./common.h"

#include "../utils/include/selector.h"
#include "../utils/include/buffer.h"
#include "../pop3/include/pop3.h"
#include "../pop3/include/users.h"
#include "../utils/include/logger.h"
#include "../utils/include/args.h"

#endif
