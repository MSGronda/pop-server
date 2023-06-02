#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./include/parser.h"
#include "./logger.h"

#define IS_MULTILINE(command, args_count) (command->type == CMD_CAPA       \
                ||  (command->type == CMD_LIST && (args_count == 0))         \
                ||  (command->type == CMD_RETR && (args_count == 1))         \
                )

typedef struct command_info {
    command_type type;
    char *    name;
    int       len;
    int       min_args;
    int       max_args;
} command_info;

static const command_info all_command_info[] = {
        {
                .type = CMD_USER, .name = "USER", .len = 4, .min_args = 1, .max_args = 1,
        } , {
                .type = CMD_PASS, .name = "PASS", .len = 4, .min_args = 1, .max_args = 1,
        } , {
                .type = CMD_RETR, .name = "RETR", .len = 4, .min_args = 1, .max_args = 1,
        } , {
                .type = CMD_LIST, .name = "LIST", .len = 4, .min_args = 0, .max_args = 1,
        } , {
                .type = CMD_CAPA, .name = "CAPA", .len = 4, .min_args = 0, .max_args = 0,
        } , {
                .type = CMD_QUIT, .name = "QUIT", .len = 4, .min_args = 0, .max_args = 0,
        },  {
                .type = CMD_DELE, .name = "DELE", .len = 4, .min_args = 1, .max_args = 1,
        },  {
                .type = CMD_NOOP, .name = "NOOP", .len = 4, .min_args = 0, .max_args = 0,
        },  {
                .type = CMD_STAT, .name = "STAT", .len = 4, .min_args = 0, .max_args = 0,
        }
};

static void command_recognition (input_parser * parser, char c, bool * finished, command_instance * current_command);
static void with_arguments_state (input_parser * parser, char c, bool * finished, command_instance * current_command);
static void crlf_state (input_parser * parser, char c, bool * finished, command_instance * current_command);
static void error_state (input_parser * parser, char c, bool * finished, command_instance * current_command);


static void handle_parsed(command_instance * current_command, input_parser * parser, bool * finished, bool not_match);

void parser_init(input_parser * parser) {
    parser->state          = COMMAND_TYPE;
    parser->line_size       = 0;
    parser->arg_length      = 0;
    parser->args_count      = 0;
    parser->is_expecting_new_arg = false;
}

command_state parser_feed(input_parser * parser, const char c, bool * finished) {
    command_instance * current_command = &parser->current_command;

    if(parser->line_size == 0) {
        current_command->type = CMD_NOT_RECOGNIZED;
        current_command->argument = NULL;
        parser->crlf_state   = 0;
        parser->args_count    = 0;
        parser->invalid_size = 0;
        for(int i = 0; i < ALL_CMD_SIZE; i++)
            parser->invalid_type[i] = false;
    }

    if (parser->state > COMMAND_ERROR){ // Nunca deberia entrar en este caso pero se deja por claridad
        log(ERROR,"Command parser not reconize state: %d", parser->state);
    }
    else{
        if(parser->state == COMMAND_TYPE ){
            command_recognition(parser, c, finished, current_command);
        }else if( parser->state == COMMAND_ARGS){
            with_arguments_state(parser, c, finished, current_command);
        }else if( parser->state == COMMAND_CRLF){
            crlf_state(parser, c, finished, current_command);
        }else{  //type == COMMAND_ERROR
            error_state(parser, c, finished, current_command);
        }
    }

    if(parser->line_size++ == MAX_MSG_SIZE || (parser->state == COMMAND_ARGS && parser->arg_length > MAX_ARG_LENGTH)){
        parser->state = COMMAND_ERROR;
        printf("entre aca\n");
    }
    return parser->state;
}

command_state parser_consume(input_parser * parser, buffer* buffer, bool * finished, size_t * n_consumed) {
    command_state state = parser->state;
    size_t n = 0;
    while(buffer_can_read(buffer)) {
        n++;
        const uint8_t c = buffer_read(buffer);
        state = parser_feed(parser, c, finished);
        if(*finished) {
            break;
        }
    }
    *n_consumed = n;
    return state;
}


static void handle_parsed(command_instance * current_command, input_parser * parser, bool * finished, bool not_match) {

    if(not_match) {
        current_command->type = CMD_NOT_RECOGNIZED;
        if(current_command->argument != NULL) {
            free(current_command->argument);
            current_command->argument = NULL;
        }
    }

    current_command->is_multi = IS_MULTILINE(current_command, parser->args_count);

    parser->state     = COMMAND_TYPE;
    parser->line_size  = -1;
    parser->arg_length--;
    *finished = true;

}

// modules for parser_feed's switch
static void command_recognition (input_parser * parser, const char c, bool * finished, command_instance * current_command) {
    if(c != '\n') {
        for(int i = 0; i < ALL_CMD_SIZE; i++) {
            if(!parser->invalid_type[i]) {
                if(toupper(c) != all_command_info[i].name[parser->line_size]) {
                    parser->invalid_type[i] = true;
                    parser->invalid_size++;
                } else if(parser->line_size == all_command_info[i].len-1) {
                    current_command->type = all_command_info[i].type;
                    parser->arg_length = 0;
                    if(all_command_info[i].max_args > 0) {
                        if (current_command->argument == NULL) {
                            current_command->argument = calloc(MAX_ARG_LENGTH + 1,sizeof(char));    //NULL TERMINATED
                        }
                        else{
                            memset(current_command->argument,0,MAX_ARG_LENGTH + 1);
                        }
                        parser->state = COMMAND_ARGS;
                    } else
                        parser->state = COMMAND_CRLF;
                    break;
                }
            }
            if(parser->invalid_size == ALL_CMD_SIZE)
                parser->state = COMMAND_ERROR;
        }
    } else
        handle_parsed(current_command, parser, finished, true);
}

static void with_arguments_state (input_parser * parser, const char c, bool * finished, command_instance * current_command) {

    // Espacio indica que deberia venir argumento
    if(c == ' ' && (current_command->type != CMD_PASS || parser->args_count == 0)) {    //el chequeo extra es para que PASS pueda recibir el argumento con espacios entre medio
        parser->is_expecting_new_arg = true;
    }
    // Leyendo un argumento
    else if((c != '\r' && c != '\n' ) || ( c == ' ' && current_command->type == CMD_PASS && parser->args_count == 1)) {
        if (parser->is_expecting_new_arg){
            if(parser->args_count >= all_command_info[current_command->type].max_args)
                parser->state = COMMAND_ERROR;
            else if(parser->arg_length == 0)
                parser->arg_length++;
            parser->args_count++;
            parser->is_expecting_new_arg = false;
        }
        parser->crlf_state = 0;
        if(parser->arg_length == 0)
            parser->state = COMMAND_ERROR;
        else {
            if(all_command_info[parser->current_command.type].max_args > 0)
                (current_command->argument)[parser->arg_length-1] = c;
                
            parser->arg_length++;
        }
    }
    else if(c == '\r') {
        parser->crlf_state = 1;
        if(all_command_info[parser->current_command.type].max_args > 0)
            (current_command->argument)[parser->arg_length > 0 ? parser->arg_length-1: 0] = 0;     //username null terminated
        if( parser->args_count >= all_command_info[current_command->type].min_args && parser->args_count <= all_command_info[current_command->type].max_args){
            parser->state = COMMAND_CRLF;
        } else{
            parser->state = COMMAND_ERROR;
        }
    }
    // Es imposible que c != '\n' si llegamos aca, es un tema de claridad
    else if(c == '\n' && parser->crlf_state == 1) {
        parser->crlf_state = 2;
        if(all_command_info[parser->current_command.type].max_args > 0)
            (current_command->argument)[parser->arg_length-1] = 0;     //username null terminated
        if(all_command_info[current_command->type].min_args <= parser->args_count && parser->args_count <= all_command_info[current_command->type].max_args) {
            handle_parsed(current_command, parser, finished, false);
        } else
            handle_parsed(current_command, parser, finished, true);
    } else {
        parser->crlf_state = 0;
        parser->state = COMMAND_ERROR;
    }
}

static void crlf_state (input_parser * parser, const char c, bool * finished, command_instance * current_command) {
    if(c == '\r' && parser->crlf_state == 0) {
        parser->crlf_state = 1;
    } else if(c == '\n' && parser->crlf_state == 1){
        handle_parsed(current_command, parser, finished, false);
    } else {
        if( c != ' ')
            parser->state = COMMAND_ERROR;
    }
}

static void error_state (input_parser * parser, const char c, bool * finished, command_instance * current_command) {
    if(c == '\r' && parser->crlf_state == 0) {
        parser->crlf_state = 1;
    } else if(c == '\n'){
        handle_parsed(current_command, parser, finished, true);
    }else{
        parser->crlf_state = 0;
    }
}