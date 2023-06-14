#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../utils/include/logger.h"
#include "../../utils/include/buffer.h"

#define MAX_MSG_SIZE 512
#define MAX_ARG_LENGTH 40
#define ALL_CMD_SIZE  10

typedef enum command_type {
    CMD_NOT_RECOGNIZED     = -1,
    CMD_USER               =  0,
    CMD_PASS               =  1,
    CMD_RETR               =  2,
    CMD_LIST               =  3,
    CMD_CAPA               =  4,
    CMD_QUIT               =  5,
    CMD_DELE               =  6,
    CMD_NOOP               =  7,
    CMD_STAT               =  8,
    CMD_RSET               =  9,
} command_type;

typedef struct command_instance {
    command_type    type;
    char *       argument;
    bool         is_multi;
} command_instance;

typedef enum command_state {
    RECOGNITION_STATE,
    WITH_ARGS_STATE,
    NO_ARGS_STATE,
    COMMAND_ERROR_STATE,
} command_state;

typedef struct input_parser {
    int                 line_size;
    int                 correctly_formed;  //0 NONE, 1 \r, 2 \n
    int                 arg_length;
    int                 args_count;
    int                 invalid_size;
    bool                invalid_type[ALL_CMD_SIZE];     //completa arreglo a medida que comando no coincida
    command_state       state;
    command_instance    current_command;
    bool                is_expecting_new_arg;
} input_parser;

// Inicializa el parser
void parser_init(input_parser * parser);

// Consume de a 1 los caracteres que le pasas. Va guardando un estado interno del comando posible actual
command_state parser_feed(input_parser * parser, const char c, bool * finished);

// Consume de a multiples caracteres. Se le pasa un buffer de la libreria. Va guardando un estado interno del comando posible actual
command_state parser_consume(input_parser * parser, buffer* buffer, bool * finished,size_t * n_consumed);

#endif