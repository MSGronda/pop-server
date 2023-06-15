#ifndef ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdio.h> 
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>

#include "../../server/include/server.h"


void parse_args(int argc, char * argv[], struct pop3_server_state * pop3_server_state);

#endif