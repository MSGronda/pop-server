#include "./include/mails.h"

// TODO: change to pop3_server settings
#define DIR_BASE "/mnt/c/Users/Mbox1/Desktop/Protos/PopServer/maildir"
//#define DIR_BASE "/home/machi/protos/pop-server/src/pop3/maildir"

#define MAX_NAME_SIZE 256

static unsigned long check_mail(buffer * write_buffer, user_mail_info * mail_info, unsigned long mail_num);

unsigned int initialize_mails(user_mail_info * mail_info, char * username){

    // EXP: generamos un string que tenga como base el directorio del usuario
    size_t dir_base_len = sizeof(DIR_BASE);
    char * file_name = malloc(dir_base_len + 2 * MAX_NAME_SIZE + 2);   // Tenemos en cuenta el nombre del directorio del usuario y el nombre del archivo
    if(file_name == NULL){
        return ERROR_ALLOC;
    }
    int user_base_len = snprintf(file_name, dir_base_len + 2 * MAX_NAME_SIZE + 2, "%s/%s/", DIR_BASE, username);

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
            char * pos = malloc(len + 1);
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
        return 0;
    }
    return 1;
}