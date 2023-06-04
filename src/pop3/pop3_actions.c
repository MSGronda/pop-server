#include "./include/pop3_actions.h"

// = = = =  Actions = = = =  
void pop3_invalid_command_action(client_connection_data * client_data);
void pop3_stat(client_connection_data * client_data);
void pop3_user(client_connection_data * client_data);
void pop3_pass(client_connection_data * client_data);
void pop3_capa(client_connection_data * client_data);
void pop3_quit(client_connection_data * client_data);
void pop3_retr(client_connection_data * client_data);
void pop3_dele(client_connection_data * client_data);
void pop3_noop(client_connection_data * client_data);
void pop3_list(client_connection_data * client_data);


// EXP: puntero a funcion que ejecuta la accion
typedef void (*pop3_action)(client_connection_data * );

typedef struct pop3_action_type{
     command_type type;
     pop3_action handle;
}pop3_action_type;


// EXP: contienen los comandos validos para cada estado / subestado
static const pop3_action_type auth_ini_actions[] = {
     {.type = CMD_CAPA, .handle = &pop3_capa},
     {.type = CMD_USER, .handle = &pop3_user},
     {.type = CMD_QUIT, .handle = &pop3_quit},

     // TODO: !!!!!!REMOVE THESE!!!!!!
     {.type = CMD_PASS, .handle = &pop3_pass},
     {.type = CMD_RETR, .handle = &pop3_retr},
     {.type = CMD_LIST, .handle = &pop3_list},
     {.type = CMD_QUIT, .handle = &pop3_quit},
     {.type = CMD_DELE, .handle = &pop3_dele},
     {.type = CMD_NOOP, .handle = &pop3_noop},
     {.type = CMD_STAT, .handle = &pop3_stat},
};
static const pop3_action_type auth_password_actions[] = {
     {.type = CMD_PASS, .handle = &pop3_pass},
     {.type = CMD_QUIT, .handle = &pop3_quit},
};
static const pop3_action_type transaction_actions[] = {
     {.type = CMD_RETR, .handle = &pop3_retr},
     {.type = CMD_LIST, .handle = &pop3_list},
     {.type = CMD_QUIT, .handle = &pop3_quit},
     {.type = CMD_DELE, .handle = &pop3_dele},
     {.type = CMD_NOOP, .handle = &pop3_noop},
     {.type = CMD_STAT, .handle = &pop3_stat},
};


pop3_action find_action(command_type command, const pop3_action_type * actions, size_t size){
     for(size_t i=0; i<size; i++) {
          if(actions[i].type == command) {
               return actions[i].handle;
          }
     }
     return &pop3_invalid_command_action;
}


void pop3_action_handler(client_connection_data * client_data, command_state cmd_state) {
     command_type command = client_data->command_parser.current_command.type;

     if(cmd_state == COMMAND_ERROR_STATE || command == CMD_NOT_RECOGNIZED) {
          pop3_invalid_command_action(client_data);
          return;
     }

     // EXP: busco y ejecuto el comando adecuado. si no es un comando valido para dicho estado, se ejecuta el pop3_invalid_command_action
     switch(client_data->state){
          case AUTH_INI:
               find_action(command, auth_ini_actions, sizeof(auth_ini_actions)/sizeof(pop3_action_type))(client_data);
               break;
          case AUTH_PASSWORD:
               find_action(command, auth_password_actions, sizeof(auth_password_actions)/sizeof(pop3_action_type))(client_data);
               break;
          case TRANSACTION:
               find_action(command, transaction_actions, sizeof(transaction_actions)/sizeof(pop3_action_type))(client_data);
               break;
          default:
          // TODO: error?
               break;
     }
}


void pop3_invalid_command_action(client_connection_data * client_data) {
     char * msg = "-ERR invalid command\n";
     size_t msg_len = strlen(msg);

     size_t max_write;
     uint8_t * write_dir = buffer_write_ptr(&client_data->write_buffer,&max_write);

     if(max_write < msg_len) {
          // TODO: handle buffer doesn't have enough space error
     }
     memcpy(write_dir, msg, msg_len);
     buffer_write_adv(&client_data->write_buffer, msg_len);
}    
// *********** TODO MODULARIZE ***********
void pop3_noop(client_connection_data * client_data){
     char * msg = "+OK\n";
     size_t msg_len = strlen(msg);

     size_t max_write;
     uint8_t * write_dir = buffer_write_ptr(&client_data->write_buffer,&max_write);

     if(max_write < msg_len) {
          // TODO: handle buffer doesn't have enough space error
     }
     memcpy(write_dir, msg, msg_len);
     buffer_write_adv(&client_data->write_buffer, msg_len);
}

void pop3_stat(client_connection_data * client_data){
     char * msg = "STAT\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}

void pop3_user(client_connection_data * client_data){
     char * msg = "USER\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}

void pop3_pass(client_connection_data * client_data){
     char * msg = "PASS\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}

void pop3_capa(client_connection_data * client_data){
     char * msg = "CAPA\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}

void pop3_quit(client_connection_data * client_data){
     char * msg = "QUIT\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}
void pop3_retr(client_connection_data * client_data){
     char * msg = "RETR\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}
void pop3_dele(client_connection_data * client_data){
     char * msg = "DELE\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}
void pop3_list(client_connection_data * client_data){
     char * msg = "LIST\n";

     for(int i=0; msg[i]!=0; i++) {
          buffer_write(&client_data->write_buffer, msg[i]);
     }
}
