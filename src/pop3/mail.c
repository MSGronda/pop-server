#include "./include/mails.h"

// TODO: change to pop3_server settings
#define DIR_BASE "/mnt/c/Users/Mbox1/Desktop/Protos/PopServer/maildir"
//#define DIR_BASE "/home/machi/protos/pop-server/src/pop3/maildir"

#define MAX_NAME_SIZE 256

static unsigned long check_mail(buffer * write_buffer, user_mail_info * mail_info, unsigned long mail_num);

int user_file_name(char ** file_name, char * username){
    // EXP: generamos un string que tenga como base el directorio del usuario
    size_t dir_base_len = sizeof(DIR_BASE);
    *file_name = calloc(dir_base_len + 2 * MAX_NAME_SIZE + 2, sizeof(char));   // Tenemos en cuenta el nombre del directorio del usuario y el nombre del archivo
    if(file_name == NULL){
        return ERROR_ALLOC;
    }
    int user_base_len = snprintf(*file_name, dir_base_len + 2 * MAX_NAME_SIZE + 2, "%s/%s/", DIR_BASE, username);

    return user_base_len;
}

unsigned int initialize_mails(user_mail_info * mail_info, char * username){

    char * file_name;
    int user_base_len = user_file_name(&file_name, username);

    // EXP: abrimos el directorio
    DIR * dir = opendir(file_name);
    if(dir == NULL){
        printf("Error opening dir\n");
        return ERROR_OPENDIR;
    }
    struct dirent * dir_info = readdir(dir);

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

unsigned long convert_mail_num(char * arg){
    for(int i=0; arg[i] != 0; i++){
        if(!isdigit(arg[i])){
            return LONG_MAX;
        }
    }
    return strtoul(arg, NULL, 10);
}

int list_mail(buffer * write_buffer, user_mail_info * mail_info, char * arg){
    // EXP: se considera single-line esta respuesta por ende, podemos esribir la respuesta de una

    unsigned long mail_num = convert_mail_num(arg);

    if(check_mail(write_buffer, mail_info, mail_num)) {
        size_t max_len;
        uint8_t * ptr = buffer_write_ptr(write_buffer, &max_len);
            
        int len = snprintf((char *)ptr, max_len, "+OK %ld %ld\r\n", mail_num, mail_info->mails[mail_num - 1].octets);

        buffer_write_adv(write_buffer, len);
    }
    return true;
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

// TODO: importante ver si podemos hacer esto de otra forma

#define LIST_LINE_LEN(i, octets) (num_string_size(i) + num_string_size(octets) + 3)
#define FIRST_LINE_LIST_LEN(count, octets)  (num_string_size(count) + num_string_size(octets) + 25)

bool can_write_list_line(unsigned int i, size_t octets, size_t max_len){
    // EXP: el 3 es el espacio, el \r y el \n
    return LIST_LINE_LEN(i, octets) + 3 < max_len;
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
            if(!can_write_list_line(i,mail_info->mails[i].octets, max_len)){
                return false;
            }

            int len = snprintf((char *)ptr, max_len, "%d %ld\r\n", i + 1, mail_info->mails[i].octets);

            buffer_write_adv(write_buffer, len);

            command->bytes_written += len;
        }
    }
    return true;
}

void stat_mailbox(buffer * write_buffer, user_mail_info * mail_info) {
    size_t max_len;
    uint8_t * ptr = buffer_write_ptr(write_buffer, &max_len);
        
    int length = snprintf((char *)ptr, max_len, "+OK %ld %ld\r\n", mail_info->current_count, mail_info->total_octets);

    buffer_write_adv(write_buffer, length);
}

void delete_mail(buffer * write_buffer, user_mail_info * mail_info, char * arg) {
    unsigned long mail_num = convert_mail_num(arg);

    if(check_mail(write_buffer, mail_info, mail_num)) {
        mail_info->mails[mail_num - 1].state = 0;
        mail_info->current_count--;
        mail_info->total_octets -= mail_info->mails[mail_num - 1].octets;


        char * msg = "+OK message deleted\r\n";
        size_t len = strlen(msg);

        buffer_write_n(write_buffer, msg, len);
    }
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


static unsigned long check_mail(buffer * write_buffer, user_mail_info * mail_info, unsigned long mail_num) {
    if(mail_num == ULONG_MAX || mail_info->mail_count < mail_num || mail_num == 0 || mail_info->mails[mail_num - 1].state == 0){
        char * msg = "-ERR no such message\r\n";
        size_t len = strlen(msg);

        buffer_write_n(write_buffer, msg, len);
        return false;
    }
    return true;
}



// = = = = = = =<   RETR   >= = = = = = = 

#define ATTACHMENT_MAIL(key) ((struct user_mail_info *)(key)->data)

void mail_read_handler(struct selector_key *key){
    user_mail_info * mail_info = ATTACHMENT_MAIL(key);

    if(!buffer_can_write(&mail_info->retrive_buffer)){
        return;
    }
    size_t read_max;   
    uint8_t * buffer = buffer_write_ptr(&mail_info->retrive_buffer, &read_max);

    // EXP: me muevo hasta donde puede leer la ultima vez asi continuo desde ahi
    off_t seek = lseek(mail_info->filed_fd, mail_info->bytes_read, SEEK_SET);

    if(seek == -1){
        printf("Error lseek\n");
        mail_info->finished_reading = true;
        selector_unregister_fd(key->s, mail_info->filed_fd);
        // TODO: handle error
        return;
    }

    ssize_t recieved_count = read(mail_info->filed_fd, buffer, read_max);

    if(recieved_count == -1) {
        printf("Error read. ERRNO: %d\n", errno);
        mail_info->finished_reading = true;
        selector_unregister_fd(key->s, mail_info->filed_fd);
        // TODO: handle error
        return;
    }

    // EXP: llegue al final del archivo, marco como si termine.
    if(recieved_count == 0) {
        mail_info->finished_reading = true;
        selector_unregister_fd(key->s, mail_info->filed_fd);
    }


    buffer_write_adv(&mail_info->retrive_buffer, recieved_count);
    mail_info->bytes_read += recieved_count;
}

static const struct fd_handler mail_handlers ={
    .handle_read = &mail_read_handler,
    .handle_write = NULL,
    .handle_close = NULL,
    .handle_block = NULL,
};

// EXP: por lo general intentamos pasarlo lo minimo indespensable a las funciones
// EXP: pero en este caso requiere tantos parametros que es mejor solo pasarle la estructura entera
int retrieve_mail(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);
    unsigned long mail_num = convert_mail_num(client_data->command_parser.current_command.argument);

    if(!check_mail(&client_data->write_buffer, &client_data->mail_info, mail_num)) {
        printf("doesnt exist\n");
        return true;
    }
    
    if(client_data->mail_info.filed_fd == 0) {
        char * file_name;
        int user_base_len = user_file_name(&file_name, client_data->username);

        if(user_base_len == ERROR_ALLOC){
            printf("Error user_file_name\n");
            // TODO: handle error
            return true;
        }
        sprintf(file_name + user_base_len,"%s", client_data->mail_info.mails[mail_num - 1].name);

        FILE * file = fopen(file_name,"r");
        if(file == NULL){
            printf("Error fopen, errno:%d\n",errno);
            // TODO: handle error
            return true;
        }

        free(file_name);

        int file_fd = fileno(file);
        if(file_fd == -1){
            printf("Error fileno\n");
            // TODO: handle error
            return true;
        }

        client_data->mail_info.filed_fd = file_fd;
        client_data->mail_info.bytes_read = 0;

        if(selector_register(key->s, file_fd, &mail_handlers, OP_READ, &client_data->mail_info) != SELECTOR_SUCCESS){
            printf("Error registering selector to read file\n");
            // TODO: handle error
            return true;
        }

        // EXP: no termine, tengo que seguir probando (?)
        return false;
    }

    if(buffer_can_read(&client_data->mail_info.retrive_buffer) && buffer_can_write(&client_data->write_buffer)){    
        size_t read_max;   
        uint8_t * read_buffer = buffer_read_ptr(&client_data->mail_info.retrive_buffer, &read_max);

        size_t write_max;   
        uint8_t * write_buffer = buffer_write_ptr(&client_data->write_buffer, &write_max);

        // TODO: byte stuffing (!!!!)

        size_t min = write_max > read_max ? read_max : write_max;

        memcpy(write_buffer, read_buffer, min);

        buffer_read_adv(&client_data->mail_info.retrive_buffer, min);
        buffer_write_adv(&client_data->write_buffer, min);
        client_data->command.bytes_written += min;

    }

    // TODO: "apagar" el fd de lectura del archivo si no puedo escribir en el buffer de salida del cliente, para no gastar recursos.

    if(client_data->mail_info.finished_reading){

        // fclose(file); // TODO: tengo que cerrar el archivo y el fd o puedo hacer solo uno (?)
        close(client_data->mail_info.filed_fd);

        client_data->mail_info.filed_fd = 0;
        client_data->mail_info.bytes_read = 0;
        client_data->mail_info.finished_reading = 0;

        return true;
    }


    return false;
}