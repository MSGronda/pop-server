#include "./include/mails.h"

void mail_read_handler(struct selector_key *key);

// = = = = = = =<   CONSTANTES / VARIABLES ESTATICAS  >= = = = = = = 

#define MAX_NAME_SIZE 256

static const struct fd_handler mail_handlers ={
    .handle_read = &mail_read_handler,
    .handle_write = NULL,
    .handle_close = NULL,
    .handle_block = NULL,
};

// = = = = = = =<   HELPER FUNCTIONS / MACROS  >= = = = = = = 

#define VALID_MAIL(mail_num, mail_info) (mail_num != ULONG_MAX && mail_num != 0  && mail_info->mail_count >= mail_num  && mail_info->mails[mail_num - 1].state != 0)
#define ATTACHMENT_MAIL(key) ((struct user_mail_info *)(key)->data)
#define LIST_LINE_LEN(i, octets) (num_string_size(i) + num_string_size(octets) + 3)
#define FIRST_LINE_LIST_LEN(count, octets)  (num_string_size(count) + num_string_size(octets) + 25)
#define CAN_WRITE_LIST_LINE(i, octets, maxlen) (LIST_LINE_LEN(i, octets) + 3 < maxlen)

void handle_invalid_mail(buffer * write_buffer){

    // EXP: no se revela mas informacion de la que se deberia
    char * msg = "-ERR no such message\r\n";
    size_t len = strlen(msg);

    buffer_write_n(write_buffer, msg, len);
}

int user_file_name(char ** file_name, char * username, char * maildir){
    // EXP: generamos un string que tenga como base el directorio del usuario
    // EXP: este tiene suficient espacio para poder escribir el nombre del usuario dentro del string
    size_t dir_base_len = sizeof(maildir);
    *file_name = calloc(dir_base_len + 2 * MAX_NAME_SIZE + 2, sizeof(char));   // Tenemos en cuenta el nombre del directorio del usuario y el nombre del archivo
    if(file_name == NULL){
        return ERROR_ALLOC;
    }
    int user_base_len = snprintf(*file_name, dir_base_len + 2 * MAX_NAME_SIZE + 2, "%s/%s/", maildir, username);

    return user_base_len;
}

unsigned long convert_mail_num(char * arg){
    for(int i=0; arg[i] != 0; i++){
        if(!isdigit(arg[i])){
            return LONG_MAX;
        }
    }
    return strtoul(arg, NULL, 10);
}

unsigned int num_string_size(size_t num){
    if(num == 0){
        return 1;
    }
    size_t n = num;
    unsigned i=0;
    while(n != 0){
        i++;
        n /= 10;
    }
    return i;
}

bool write_multiline_end(buffer * write_buffer){
    size_t write_max;
    uint8_t * ptr = buffer_write_ptr(write_buffer, &write_max);
    char * delimiter = "\r\n.\r\n";                                 // TODO: CHECK THIS WITH JOSE
    size_t len = strlen(delimiter);

    // EXP: solo escribimos si tenemos espacio para escribir el mensaje completo 
    if(write_max < len){
        return false;
    }
    
    memcpy(ptr, delimiter, len);

    buffer_write_adv(write_buffer, len);
    return true;
}

// = = = = = = =<   INICIALIZACION Y DESTRUCCION  >= = = = = = = 

unsigned int initialize_mails(user_mail_info * mail_info, char * username, char * maildir){

    char * file_name;
    int user_base_len = user_file_name(&file_name, username, maildir);

    DIR * dir = opendir(file_name);
    if(dir == NULL){
        return ERROR_DIR;
    }

    // EXP: se consider readdir como no bloqueante. 
    errno = 0;
    struct dirent * dir_info = readdir(dir);

    // EXP: readdir puede leer sin error y retornar NULL, por lo que, debemos usar errno para distinguir si es error
    if(dir_info == NULL && errno != 0) {
        return ERROR_DIR;
    }

    struct stat file_info;
    unsigned i = 0;
    size_t total_octets = 0;

    // EXP: iteramos por los archivos en el directorio, guardando los filenames y tamanos
    while(dir_info != NULL && i<MAX_MAILS) {
        if(strcmp(dir_info->d_name, ".")!=0 && strcmp(dir_info->d_name, "..")!=0){

            // EXP: copio el nombre del file
            size_t len = strlen(dir_info->d_name);
            char * pos = calloc(len + 1, sizeof(char));
            if(pos == NULL){
                return ERROR_ALLOC;
            }
            mail_info->mails[i].name = pos;

            strcpy(mail_info->mails[i].name, dir_info->d_name);

            // EXP: tengo que conseguir los stats del archivo
            strcpy(file_name + user_base_len, dir_info->d_name);
            int stat_status = stat(file_name, &file_info);

            // EXP: consigo el tamano del archivo
            if(stat_status == -1){
                return ERROR_STAT;
            }
            mail_info->mails[i].octets = file_info.st_size;
            mail_info->mails[i].state = 1;

            total_octets += file_info.st_size;

            i++;
        }
        
        dir_info = readdir(dir);
    }
    mail_info->mail_count = i;
    mail_info->current_count = i;
    mail_info->total_octets = total_octets;

    closedir(dir);

    return MAILS_SUCCESS;
}

void free_mail_info(struct selector_key *key){
    client_connection_data * client_data = ATTACHMENT(key);

    client_data->mail_info.finished_reading = true;
    if(client_data->mail_info.filed_fd != 0){
        selector_unregister_fd(key->s, client_data->mail_info.filed_fd);
        close(client_data->mail_info.filed_fd);
    }
}

// = = = = = = =<   COMANDOS  >= = = = = = = 

int list_mail(buffer * write_buffer, user_mail_info * mail_info, char * arg){
    // EXP: se considera single-line esta respuesta por ende, podemos esribir la respuesta de una

    unsigned long mail_num = convert_mail_num(arg);

    if(!VALID_MAIL(mail_num, mail_info)){
        handle_invalid_mail(write_buffer);
        return true;
    }

    size_t max_len;
    uint8_t * ptr = buffer_write_ptr(write_buffer, &max_len);
        
    int len = snprintf((char *)ptr, max_len, "+OK %ld %ld\r\n", mail_num, mail_info->mails[mail_num - 1].octets);

    buffer_write_adv(write_buffer, len);
    
    return true;
}

int list_mails(buffer * write_buffer, user_mail_info * mail_info, running_command * command){
    // EXP: chequeamos si podemos escribir una linea entera del comando list. EJ: 3 430
    // EXP: nos manejamos por lineas enteras para facilitar la logica.

    // EXP: la primera linea la suponemos single line (de la misma forma que en los otros comandos), al llegar el cliente con write_buffer vacio
    if(command->bytes_written == 0){
        size_t max_len;
        uint8_t * response = buffer_write_ptr(write_buffer, &max_len);
        int length = snprintf((char *)response,  max_len,"+OK %ld messages (%ld octets)\r\n", mail_info->current_count,  mail_info->total_octets);
        buffer_write_adv(write_buffer, length);  
        command->bytes_written += length;
    }


    // EXP: voy hasta donde llegue anteriormente
    // EXP: se hizo de esta forma para no tener que guardar en el estado del usuario algo especifico del este comando
    unsigned int i=0;
    size_t previous_sent = FIRST_LINE_LIST_LEN(mail_info->current_count, mail_info->total_octets);      // EXP: empezandos contando despues de la primera linea 
    while(previous_sent < command->bytes_written  && i < mail_info->mail_count) {
        if(mail_info->mails[i].state != 0){
            previous_sent += LIST_LINE_LEN(i, mail_info->mails[i].octets);
        }
        i++;
    }

    // EXP: itero por los mails NO ELIMINADOS y escribo al buffer
    for(; i < mail_info->mail_count; i++) {
        if(mail_info->mails[i].state != 0){
            size_t max_len;
            uint8_t * ptr = buffer_write_ptr(write_buffer, &max_len);

            // EXP: me quede sin espacio, marco como si no termine    
            if(!CAN_WRITE_LIST_LINE(i, mail_info->mails[i].octets, max_len)){
                return false;
            }

            int len = snprintf((char *)ptr, max_len, "%d %ld\r\n", i + 1, mail_info->mails[i].octets);

            buffer_write_adv(write_buffer, len);

            command->bytes_written += len;
        }
    }

    return write_multiline_end(write_buffer);
}

void stat_mailbox(buffer * write_buffer, user_mail_info * mail_info) {
    size_t max_len;
    uint8_t * ptr = buffer_write_ptr(write_buffer, &max_len);
        
    int length = snprintf((char *)ptr, max_len, "+OK %ld %ld\r\n", mail_info->current_count, mail_info->total_octets);

    buffer_write_adv(write_buffer, length);
}

void delete_mail(buffer * write_buffer, user_mail_info * mail_info, char * arg) {
    unsigned long mail_num = convert_mail_num(arg);

    if(!VALID_MAIL(mail_num, mail_info)){
        handle_invalid_mail(write_buffer);
    }

    mail_info->mails[mail_num - 1].state = 0;
    mail_info->current_count--;
    mail_info->total_octets -= mail_info->mails[mail_num - 1].octets;

    char * msg = "+OK message deleted\r\n";
    size_t len = strlen(msg);

    buffer_write_n(write_buffer, msg, len);

}   

void restore_mail(buffer * write_buffer, user_mail_info * mail_info) {
    for(unsigned i=0; i < mail_info->mail_count; i++) {
        if(mail_info->mails[i].state == 0){
            mail_info->mails[i].state = 1;
            mail_info->current_count++;
            mail_info->total_octets += mail_info->mails[i].octets;
        }
    }

    size_t max_len;
    uint8_t * response = buffer_write_ptr(write_buffer, &max_len);
        
    int length = snprintf((char *)response, max_len, "+OK maildrop has %ld messages (%ld octets)\r\n",  mail_info->current_count, mail_info->total_octets);

    buffer_write_adv(write_buffer, length);  
}



// = = = = = = =<   RETR   >= = = = = = = 

void init_stuffing_parser(stuffing_parser * parser){
    parser->prev = 0;
    parser->second_prev = 0;
    parser->third_prev = 0;
}

void stuffing_consume(stuffing_parser * parser, uint8_t c){
    parser->third_prev = parser->second_prev;
    parser->second_prev = parser->prev;
    parser->prev = c;
}

bool needs_stuffing(stuffing_parser * parser){
    return parser->third_prev == '\r' && parser->second_prev == '\n' && parser->prev == '.';
}

void transfer_bytes(buffer * retrive_buffer, buffer * write_buffer, stuffing_parser * parser){
    size_t read_max, write_max;
    uint8_t * read_addr = buffer_read_ptr(retrive_buffer, &read_max);
    uint8_t * write_addr = buffer_write_ptr(write_buffer, &write_max);

    unsigned i=0, j=0;
    while(i < read_max && j < write_max){
        if(needs_stuffing(parser)){
            write_addr[j++] = '.';
            stuffing_consume(parser, read_addr[i]);
        }
        else{
            write_addr[j] = read_addr[i];
            stuffing_consume(parser, read_addr[i]);
            i++, j++;
        }
    }
    buffer_read_adv(retrive_buffer, i);
    buffer_write_adv(write_buffer, j);
}


void mail_read_handler(struct selector_key *key){
    char * error_msg;
    user_mail_info * mail_info = ATTACHMENT_MAIL(key);

    if(!buffer_can_write(&mail_info->retrive_buffer)){
        return;
    }

    size_t read_max;   
    uint8_t * buffer = buffer_write_ptr(&mail_info->retrive_buffer, &read_max);

    // EXP: me muevo hasta donde puede leer la ultima vez asi continuo desde ahi
    off_t seek = lseek(mail_info->filed_fd, mail_info->bytes_read, SEEK_SET);

    if(seek == -1){
        ERROR_CATCH("Error lseek of mail file.", error)
    }

    ssize_t recieved_count = read(mail_info->filed_fd, buffer, read_max);

    if(recieved_count == -1) {
        ERROR_CATCH("Error reading from mail file.", error)
    }

    // EXP: llegue al final del archivo, marco como si termine.
    if(recieved_count == 0) {
        mail_info->finished_reading = true;
        selector_unregister_fd(key->s, mail_info->filed_fd);
    }


    buffer_write_adv(&mail_info->retrive_buffer, recieved_count);
    mail_info->bytes_read += recieved_count;

    return;

error:
    log(DEBUG, "%s", error_msg)
    mail_info->finished_reading = true;
    selector_unregister_fd(key->s, mail_info->filed_fd);
}


int setup_mail_retrieval(struct selector_key *key, unsigned long mail_num, char ** error_msg, char * maildir){
    client_connection_data * client_data = ATTACHMENT(key);

    char * file_name;
    int user_base_len = user_file_name(&file_name, client_data->username, maildir);

    if(user_base_len == ERROR_ALLOC){
        *error_msg = "Error allocating for mail file name.";
        return false;
    }
    sprintf(file_name + user_base_len, "%s", client_data->mail_info.mails[mail_num - 1].name);

    FILE * file = fopen(file_name,"r");
    if(file == NULL){
        free(file_name);
        *error_msg = "Error opening mail file.";
        return false;
    }

    // EXP: libero el nombre del archivo completo ya que no lo necesito. 
    free(file_name);

    int file_fd = fileno(file);
    if(file_fd == -1){
        fclose(file);
        *error_msg = "Error converting FILE to fd.";
        return false;
    }

    client_data->mail_info.filed_fd = file_fd;
    client_data->mail_info.bytes_read = 0;


    // EXP: me suscribo a la lectura del archivo con el mismo selector que tiene a todos los clientes 
    // EXP: esto evita que un usuario lea un archivo enorme y bloquee al resto de los clientes (al ser un solo hilo)
    if(selector_register(key->s, file_fd, &mail_handlers, OP_READ, &client_data->mail_info) != SELECTOR_SUCCESS){
        close(file_fd);
        *error_msg = "Error registering to selector in RETR.";
        return false;
    }

    init_stuffing_parser(&client_data->mail_info.parser);

    return true;
}

bool finish_mail_retrieval(user_mail_info * mail_info, buffer * write_buffer){
    bool write_success = write_multiline_end(write_buffer);

    if(!write_success){
        return false;
    }

    // fclose(file); // TODO: tengo que cerrar el archivo y el fd o puedo hacer solo uno (?)
    close(mail_info->filed_fd);

    mail_info->filed_fd = 0;
    mail_info->bytes_read = 0;
    mail_info->finished_reading = 0;
    return true;
}

// EXP: por lo general intentamos pasarlo lo minimo indespensable a las funciones
// EXP: pero en este caso requiere tantos parametros que es mejor solo pasarle la estructura entera
int retrieve_mail(struct selector_key *key, char * maildir) {
    char * error_msg;
    client_connection_data * client_data = ATTACHMENT(key);

    unsigned long mail_num = convert_mail_num(client_data->command_parser.current_command.argument);
    user_mail_info * mail_info = &client_data->mail_info;

     if(!VALID_MAIL(mail_num, mail_info)){
        handle_invalid_mail(&client_data->write_buffer);
        return true;
    }

    // EXP: escribimos la pimera linea (que se considera single line)
    if(client_data->command.bytes_written == 0 && client_data->mail_info.filed_fd == 0){
        char * ini_msg = "+OK message follows\r\n";
        size_t ini_msg_len = strlen(ini_msg);
        buffer_write_n(&client_data->write_buffer, ini_msg, ini_msg_len);
    }

    // EXP: abro el archivo y me suscribo a la lectura. solo se hace 1 vez
    if(client_data->mail_info.filed_fd == 0) {
        int setup_success = setup_mail_retrieval(key, mail_num, &error_msg, maildir);

        if(!setup_success){
            goto error;
        }

        // EXP: no termine, tengo que seguir probando (?)
        return false;
    }


    if(buffer_can_read(&client_data->mail_info.retrive_buffer) && buffer_can_write(&client_data->write_buffer)){    
        transfer_bytes(&client_data->mail_info.retrive_buffer, &client_data->write_buffer, &client_data->mail_info.parser);
    }

    // TODO: "apagar" el fd de lectura del archivo si no puedo escribir en el buffer de salida del cliente, para no gastar recursos.

    if(client_data->mail_info.finished_reading){
        return finish_mail_retrieval(&client_data->mail_info, &client_data->write_buffer);
    }

    return false;
error:
    log(DEBUG, "%s", error_msg)
    handle_invalid_mail(&client_data->write_buffer);
    return true;
}