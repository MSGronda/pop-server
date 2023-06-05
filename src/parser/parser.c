#include "./include/parser.h"

static void command_recognition (input_parser * parser, char c, bool * finished, command_instance * current_command);
static void with_arguments_state (input_parser * parser, char c, bool * finished, command_instance * current_command);
static void no_arguments_state (input_parser * parser, char c, bool * finished, command_instance * current_command);
static void error_state (input_parser * parser, char c, bool * finished, command_instance * current_command);

static void handle_parsed(command_instance * current_command, input_parser * parser, bool * finished, bool not_match);
static bool is_multi(command_instance * command, int arg_count);

#define COMMAND_LENGTH 4

typedef struct command_info {
    command_type type;
    char *    name;
    int       min_args;
    int       max_args;
} command_info;

static const command_info all_command_info[] = {
        { .type = CMD_USER, .name = "USER", .min_args = 1, .max_args = 1,}, 
        { .type = CMD_PASS, .name = "PASS", .min_args = 1, .max_args = 1,},
        { .type = CMD_RETR, .name = "RETR", .min_args = 1, .max_args = 1,},
        { .type = CMD_LIST, .name = "LIST", .min_args = 0, .max_args = 1,},
        { .type = CMD_CAPA, .name = "CAPA", .min_args = 0, .max_args = 0,},
        { .type = CMD_QUIT, .name = "QUIT", .min_args = 0, .max_args = 0,},
        { .type = CMD_DELE, .name = "DELE", .min_args = 1, .max_args = 1,},
        { .type = CMD_NOOP, .name = "NOOP", .min_args = 0, .max_args = 0,},
        { .type = CMD_STAT, .name = "STAT", .min_args = 0, .max_args = 0,}
};

void parser_init(input_parser * parser) {
    //inicializa el parser - necesario tener struct de parser para poder mantener el estado en distintas llamadas
    parser->arg_length      = 0;
    parser->args_count      = 0;
    parser->line_size       = 0;
    parser->state          = RECOGNITION_STATE;
    parser->is_expecting_new_arg = false;
}

command_state parser_feed(input_parser * parser, const char c, bool * finished) {
    command_instance * current_command = &parser->current_command;

    if(parser->line_size == 0) {
        //si no recibio nada el parser antes, inicializa el comando
        current_command->type = CMD_NOT_RECOGNIZED;
        current_command->argument = NULL;
        parser->state = RECOGNITION_STATE;
        parser->correctly_formed   = 0;
        parser->args_count    = 0;
        parser->invalid_size = 0;
        for(int i = 0; i < ALL_CMD_SIZE; i++)
            //array de booleano para cada comando para marcar como posible o no
            parser->invalid_type[i] = false;
    }

    //dependiendo del estado del parser se interpreta el char de distinta forma
    if(parser->state == RECOGNITION_STATE ){         //no se determino comando
        command_recognition(parser, c, finished, current_command);
    }else if( parser->state == WITH_ARGS_STATE){   //comando con argumentos
        with_arguments_state(parser, c, finished, current_command);
    }else if( parser->state == NO_ARGS_STATE){   //comando sin argumentos
        no_arguments_state(parser, c, finished, current_command);
    }else{  //type == COMMAND_ERROR_STATE
        error_state(parser, c, finished, current_command);
    }

    if(parser->line_size++ == MAX_MSG_SIZE || (parser->state == WITH_ARGS_STATE && parser->arg_length > MAX_ARG_LENGTH)){
        parser->state = COMMAND_ERROR_STATE;
    }
    return parser->state;
}

command_state parser_consume(input_parser * parser, buffer* buffer, bool * finished, size_t * n_consumed) {
    command_state state = parser->state;
    size_t n = 0;
    while(buffer_can_read(buffer)) {
        const uint8_t c = buffer_read(buffer);
        state = parser_feed(parser, c, finished);
        n++;

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

    current_command->is_multi = is_multi(current_command, parser->args_count);

    // parser->state     = RECOGNITION_STATE;
    //es necesario poner line_size en -1 porque parser_feed
    //lo va a aumentar una vez mas
    parser->line_size  = -1;

    //arg_length se reinicia cuando se detecta el comando
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
                } else if(parser->line_size == COMMAND_LENGTH - 1) {
                    current_command->type = all_command_info[i].type;
                    parser->arg_length = 0;
                    if(all_command_info[i].max_args > 0) {
                        if (current_command->argument == NULL) {
                            current_command->argument = calloc(MAX_ARG_LENGTH + 1,sizeof(char));    //NULL TERMINATED
                        }
                        else{
                            memset(current_command->argument,0,MAX_ARG_LENGTH + 1);
                        }
                        parser->state = WITH_ARGS_STATE;
                    } else
                        parser->state = NO_ARGS_STATE;
                    break;
                }
            }
            if(parser->invalid_size == ALL_CMD_SIZE)
                parser->state = COMMAND_ERROR_STATE;
        }
    } else
        handle_parsed(current_command, parser, finished, true);
}

static void with_arguments_state (input_parser * parser, const char c, bool * finished, command_instance * current_command) {

    // Espacio indica que deberia venir argumento
    if(c == ' ' && (current_command->type != CMD_PASS || parser->args_count == 0)) {    
        //el chequeo extra es para que PASS pueda recibir el argumento con espacios entre medio
        parser->is_expecting_new_arg = true;
    }
    // Leyendo un argumento
    else if((c != '\r' && c != '\n' ) || ( c == ' ' && current_command->type == CMD_PASS && parser->args_count == 1)) {
        if (parser->is_expecting_new_arg){
            if(parser->args_count >= all_command_info[current_command->type].max_args)
                parser->state = COMMAND_ERROR_STATE;
            else if(parser->arg_length == 0)
                parser->arg_length++;
            parser->args_count++;
            parser->is_expecting_new_arg = false;
        }
        parser->correctly_formed = 0;
        if(parser->arg_length == 0)
            parser->state = COMMAND_ERROR_STATE;
        else {
            if(all_command_info[parser->current_command.type].max_args > 0)
                (current_command->argument)[parser->arg_length-1] = c;
                
            parser->arg_length++;
        }
    }
    else if(c == '\r') {
        parser->correctly_formed = 1;
        if(all_command_info[parser->current_command.type].max_args > 0)
            (current_command->argument)[parser->arg_length > 0 ? parser->arg_length-1: 0] = 0;     //username null terminated
        if( parser->args_count == 0 && all_command_info[current_command->type].min_args == 0){
            parser->state = NO_ARGS_STATE;
        }
        else if (parser->args_count < all_command_info[current_command->type].min_args || parser->args_count > all_command_info[current_command->type].max_args){
            parser->state = COMMAND_ERROR_STATE;
        }
    }
    // Es imposible que c != '\n' si llegamos aca, es un tema de claridad
    else if(c == '\n' && parser->correctly_formed == 1) {
        parser->correctly_formed = 2;
        if(all_command_info[parser->current_command.type].max_args > 0)
            (current_command->argument)[parser->arg_length-1] = 0;     //username null terminated
        if(all_command_info[current_command->type].min_args <= parser->args_count && parser->args_count <= all_command_info[current_command->type].max_args) {
            handle_parsed(current_command, parser, finished, false);
        } else
            handle_parsed(current_command, parser, finished, true);
    } else {
        parser->correctly_formed = 0;
        parser->state = COMMAND_ERROR_STATE;
        handle_parsed(current_command, parser, finished, true);
    }
}

static void no_arguments_state (input_parser * parser, const char c, bool * finished, command_instance * current_command) {
    if(c == '\r' && parser->correctly_formed == 0) {
        parser->correctly_formed = 1;
    } else if(c == '\n' && parser->correctly_formed == 1){
        handle_parsed(current_command, parser, finished, false);
    } else {
        if( c != ' ')
            parser->state = COMMAND_ERROR_STATE;
    }
}

static void error_state (input_parser * parser, const char c, bool * finished, command_instance * current_command) {
    if(c == '\r' && parser->correctly_formed == 0) {
        parser->correctly_formed = 1;
    } else if(c == '\n'){
        handle_parsed(current_command, parser, finished, true);
    }else{
        parser->correctly_formed = 0;
    }
}

static bool is_multi(command_instance * command, int arg_count){
    if( command->type == CMD_CAPA || (command->type == CMD_LIST && arg_count == 0) || (command->type == CMD_RETR && arg_count == 1) )
        return true;
    return false;

}