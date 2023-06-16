// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "./include/pop3_actions.h"

// = = = =  Actions = = = =  

// EXP: puntero a funcion que ejecuta la accion
typedef int (*pop3_action)(struct selector_key *key );

typedef struct pop3_action_type{
     command_type type;
     pop3_action handle;
}pop3_action_type;


// EXP: pasamos el selector_key (y no solo el client_data) dado que hay comandos que necesitan ese selector_key, ej: RETR
int pop3_invalid_command_action(struct selector_key *key);
int pop3_stat(struct selector_key *key);
int pop3_user(struct selector_key *key);
int pop3_pass(struct selector_key *key);
int pop3_capa(struct selector_key *key);
int pop3_quit(struct selector_key *key);
int pop3_retr(struct selector_key *key);
int pop3_dele(struct selector_key *key);
int pop3_noop(struct selector_key *key);
int pop3_list(struct selector_key *key);
int pop3_rset(struct selector_key *key);
static void send_back_to_ini(client_connection_data * client_data);

// EXP: contienen los comandos validos para cada estado / subestado
static const pop3_action_type auth_ini_actions[] = {
     {.type = CMD_CAPA, .handle = &pop3_capa},
     {.type = CMD_USER, .handle = &pop3_user},
     {.type = CMD_QUIT, .handle = &pop3_quit},
};
static const pop3_action_type auth_password_actions[] = {
     {.type = CMD_USER, .handle = &pop3_user},
     {.type = CMD_PASS, .handle = &pop3_pass},
     {.type = CMD_QUIT, .handle = &pop3_quit},
     {.type = CMD_CAPA, .handle = &pop3_capa},
};
static const pop3_action_type transaction_actions[] = {
     {.type = CMD_CAPA, .handle = &pop3_capa},
     {.type = CMD_RETR, .handle = &pop3_retr},
     {.type = CMD_LIST, .handle = &pop3_list},
     {.type = CMD_QUIT, .handle = &pop3_quit},
     {.type = CMD_DELE, .handle = &pop3_dele},
     {.type = CMD_NOOP, .handle = &pop3_noop},
     {.type = CMD_STAT, .handle = &pop3_stat},
     {.type = CMD_RSET, .handle = &pop3_rset},
};

pop3_action find_action(command_type command, const pop3_action_type * actions, size_t size){
     for(size_t i=0; i<size; i++) {
          if(actions[i].type == command) {
               return actions[i].handle;
          }
     }
     return &pop3_invalid_command_action;
}

void pop3_action_handler(struct selector_key *key) {
     client_connection_data * client_data = ATTACHMENT(key);

     command_type command = client_data->command_parser.current_command.type;
     command_state cmd_state = client_data->command_parser.state;

     pop3_action action;
     if(cmd_state == COMMAND_ERROR_STATE || command == CMD_NOT_RECOGNIZED) {
          action = &pop3_invalid_command_action;
     }
     else{
          // EXP: busco y ejecuto el comando adecuado. si no es un comando valido para dicho estado, se ejecuta el pop3_invalid_command_action
          switch(client_data->state){
               case AUTH_INI:
                    action = find_action(command, auth_ini_actions, sizeof(auth_ini_actions)/sizeof(pop3_action_type));
                    break;
               case AUTH_PASSWORD:
                    action = find_action(command, auth_password_actions, sizeof(auth_password_actions)/sizeof(pop3_action_type));
                    break;
               case TRANSACTION:
                    action = find_action(command, transaction_actions, sizeof(transaction_actions)/sizeof(pop3_action_type));
                    break;
               default:                      // TODO: error?
                    action = &pop3_invalid_command_action;
                    break;
          }
     
     }
   
     // EXP: guardamos la accion por si es multilinea y no se puede ejecutar de una.
     // EXP: cada accion tiene que manejar el valor de finished y continuar con ejecucion si es relevante. 
     client_data->command.command_num = command;
     client_data->command.finished =  action(key);

     // EXP: reseteo comando 
     if(client_data->command.finished){
          client_data->command.bytes_written = 0;
     }
}

// EXP: pasamos el selector_key (y no solo el client_data) dado que hay comandos que necesitan ese selector_key, ej: RETR
void pop3_continue_action(struct selector_key *key){
     client_connection_data * client_data = ATTACHMENT(key);

     command_type command = client_data->command.command_num;

     // EXP: solo puede ocurrir que no se manda una respuesta correcta en transaction (por RETR y LIST)
     client_data->command.finished =  find_action(command, transaction_actions, sizeof(transaction_actions)/sizeof(pop3_action_type))(key);

     // EXP: reseteo comando 
     if(client_data->command.finished){
          client_data->command.bytes_written = 0;
     }
}

int pop3_invalid_command_action(struct selector_key *key) {
     client_connection_data * client_data = ATTACHMENT(key);
     char * msg = "-ERR invalid command\r\n";
     size_t len = strlen(msg);

     send_back_to_ini(client_data);

     // en la mayoria de los casos que se usa buffer_write_n, se asume que hay suficiente espacio
     // en el buffer para escribir (al ser una longitud constant y pequena)

     buffer_write_n(&client_data->write_buffer, msg, len);
     return true;      
}    
int pop3_noop(struct selector_key *key){
     client_connection_data * client_data = ATTACHMENT(key);
     char * msg = "+OK\r\n";
     size_t len = strlen(msg);

     buffer_write_n(&client_data->write_buffer, msg, len);
     return true;
}

int pop3_capa(struct selector_key *key){
     client_connection_data * client_data = ATTACHMENT(key);
     char * msg = "+OK\r\nCAPA\r\nPIPELINING\r\nUSER\r\n.\r\n";
     size_t len = strlen(msg);
     send_back_to_ini(client_data);

     buffer_write_n(&client_data->write_buffer, msg, len);
     return true;
}

int pop3_user(struct selector_key *key){
     client_connection_data * client_data = ATTACHMENT(key);
     int index = find_user(client_data->command_parser.current_command.argument);

     char * answer = "+OK\r\n";
     size_t len = strlen(answer);
     buffer_write_n(&client_data->write_buffer, answer, len);

     if( index != -1) {
          client_data->state = AUTH_PASSWORD;
          client_data->username = malloc(client_data->command_parser.arg_length + 1);
          if(client_data->username == NULL) {
               return false;
          }
          strcpy(client_data->username, client_data->command_parser.current_command.argument);
     }
     return true;
}

int pop3_pass(struct selector_key *key){
     client_connection_data * client_data = ATTACHMENT(key);
     if (client_data->state == AUTH_PASSWORD) {
          user_status status = login_user(client_data->username, client_data->command_parser.current_command.argument);

          if (!status) {
               char * maildir = get_server_state()->folder_address;
               unsigned int resp = initialize_mails(client_data, client_data->username, maildir);

               if(resp == ERROR_DIR){
                    client_data->mail_info->is_dir_valid = false;
                    log(DEBUG, "%s", "Error loading mail data for client due to: directory error")
               }
               else if(resp != MAILS_SUCCESS) {
                    client_data->mail_info->is_dir_valid = false;
                    log(DEBUG,"Error loading mail data for client due to: %s", resp == ERROR_ALLOC ? "allocation error" : "file stat error" )
               }

               char * answer = "+OK\r\n";
               size_t len = strlen(answer);
               buffer_write_n(&client_data->write_buffer, answer, len);
               client_data->state = TRANSACTION;

               return true;
          }
          
     }
     send_back_to_ini(client_data);
     char * answer = "-ERR\r\n";
     size_t len = strlen(answer);
     buffer_write_n(&client_data->write_buffer, answer, len);
     return true;
}

int pop3_list(struct selector_key *key) {
     client_connection_data * client_data = ATTACHMENT(key);
     if(client_data->command_parser.args_count == 0){
          return list_mails(&client_data->write_buffer, client_data->mail_info, &client_data->command.bytes_written);
     }
     else{
          return list_mail(&client_data->write_buffer, client_data->mail_info, client_data->command_parser.current_command.argument);
     }
}

int pop3_retr(struct selector_key *key) {
     char * maildir = get_server_state()->folder_address;
     return retrieve_mail(key, maildir);
}


int pop3_stat(struct selector_key *key) {
     client_connection_data * client_data = ATTACHMENT(key);
     stat_mailbox(&client_data->write_buffer, client_data->mail_info);
     return true;
}

int pop3_quit(struct selector_key *key) {
     client_connection_data * client_data = ATTACHMENT(key);
     char * msg = "+OK goodbye!\r\n";

     if(client_data->state == TRANSACTION && client_data->mail_info->is_dir_valid) {
          // entering update state to delete mails
          char * maildir = get_server_state()->folder_address;
          char * user_maildir;
          int err = 0;
          int user_base_len = user_file_name(&user_maildir, client_data->username, maildir);

          for(size_t i = 0; i < client_data->mail_info->mail_count ; i++) {
               if(client_data->mail_info->mails[i].state == 0) {

                    strcpy(user_maildir + user_base_len, client_data->mail_info->mails[i].name);
                    int rm_state = remove(user_maildir);
                    //rm_state is == 0 if it was correctly removed
                    if(rm_state && !err) {

                         log(DEBUG,"Error deleting file (%s) for client with fd: %d", client_data->mail_info->mails[i].name, client_data->client_fd)

                         msg = "-ERR some messages not deleted.\r\n";
                         err = 1;
                    }
               }    
          }
     }
     int rsp_len = strlen(msg);
     buffer_write_n(&client_data->write_buffer, msg, rsp_len);
     client_data->state = CLIENT_FINISHED;
     return true;
}

int pop3_dele(struct selector_key *key) {
     client_connection_data * client_data = ATTACHMENT(key);
     delete_mail(&client_data->write_buffer, client_data->mail_info, client_data->command_parser.current_command.argument);
     return true;
}

int pop3_rset(struct selector_key *key) {
     client_connection_data * client_data = ATTACHMENT(key);
     restore_mail(&client_data->write_buffer, client_data->mail_info);
     return true;
}



static void send_back_to_ini(client_connection_data * client_data) {
     if (client_data->state == AUTH_PASSWORD) {
          free(client_data->username);
          client_data->username = NULL;
          client_data->state = AUTH_INI;
     }
}
