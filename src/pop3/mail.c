#include "./include/mails.h"

// TODO: change to pop3_server settings
#define DIR_BASE "/mnt/c/Users/Mbox1/Desktop/Protos/PopServer/maildir/"

#define MAX_NAME_SIZE 256

unsigned int initialize_mails(user_mail_info * mail_info, char * username){

    // EXP: generamos un string que tenga como base el directorio del usuario
    size_t dir_base_len = sizeof(DIR_BASE);
    char * file_name = malloc(dir_base_len + 2 * MAX_NAME_SIZE + 2);   // Tenemos en cuenta el nombre del directorio del usuario y el nombre del archivo
    if(file_name == NULL){
        return ERROR_ALLOC;
    }
    int user_base_len = sprintf(file_name, "%s/%s/", DIR_BASE, username);

    // EXP: abrimos el directorio
    DIR * dir = opendir(file_name);
    if(dir == NULL){
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

            total_octets += file_info.st_size;

            i++;
        }
       
        dir_info = readdir(dir);
    }
    mail_info->mail_count = i;
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

void list_mail(buffer * write_buffer, user_mail_info * mail_info, char * arg){
    // EXP: se considera single-line esta respuesta por ende, podemos esribir la respuesta de una

    unsigned long mail_num = convert_mail_num(arg);

    if(mail_num == ULONG_MAX || mail_info->mail_count < mail_num || mail_num == 0){
        char * msg = "-ERR no such message\r\n";
        size_t len = strlen(msg);

        buffer_write_n(write_buffer, msg, len);
        return;
    }

    size_t max_len;
    uint8_t * ptr = buffer_write_ptr(write_buffer, &max_len);

    int len = snprintf((char *)ptr, max_len, "+OK %ld %ld\r\n", mail_num, mail_info->mails[mail_num - 1].octets);

    if(len < 0){
        // TODO: handle error

        return;
    }

    buffer_write_adv(write_buffer, len);
}

void list_mails(buffer * write_buffer, user_mail_info * mail_info){
    size_t bytes_written = 0;

    char * initial = "+OK\n";
    size_t len = strlen(initial);
    buffer_write_n(write_buffer, initial, len);         // TODO: change!!

    for(unsigned i=0; i < mail_info->mail_count; i++) {
        size_t max_len;
        uint8_t * ptr = buffer_write_ptr(write_buffer, &max_len);
        
        int len = sprintf((char *)ptr, "%d %ld\n", i + 1, mail_info->mails[i].octets);

        buffer_write_adv(write_buffer, len);

        bytes_written += len;
    }
}