#include "./include/mails.h"

// TODO: change to pop3_server settings
#define DIR_BASE "/mnt/c/Users/Mbox1/Desktop/Protos/PopServer/maildir/"

#define MAX_NAME_SIZE 256

unsigned int initialize_mails(client_connection_data * client_data){

    // EXP: generamos un string que tenga como base el directorio del usuario
    size_t dir_base_len = sizeof(DIR_BASE);
    char * file_name = malloc(dir_base_len + 2 * MAX_NAME_SIZE + 2);   // Tenemos en cuenta el nombre del directorio del usuario y el nombre del archivo
    if(file_name == NULL){
        return ERROR_ALLOC;
    }
    int user_base_len = sprintf(file_name, "%s/%s/", DIR_BASE, client_data->username);

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
            client_data->mails[i].name = pos;

            strcpy(client_data->mails[i].name, dir_info->d_name);

            // EXP: tengo que conseguir los stats del archivo
            strcpy(file_name + user_base_len, dir_info->d_name);
            int stat_status = stat(file_name, &file_info);

            // EXP: consigo el tamano del archivo
            if(stat_status == -1){
                return ERROR_STAT;
            }
            client_data->mails[i].octets = file_info.st_size;

            total_octets += file_info.st_size;

            i++;
        }
       
        dir_info = readdir(dir);
    }
    client_data->mail_count = i;
    client_data->total_octets = total_octets;

    closedir(dir);

    return MAILS_SUCCESS;
}

void list_mails(client_connection_data * client_data){
    size_t bytes_written = 0;

    char * initial = "+OK\n";
    size_t len = strlen(initial);
    buffer_write_n(&client_data->write_buffer, initial, len);         // TODO: change!!

    for(unsigned i=0; i < client_data->mail_count; i++) {
        size_t max_len;
        uint8_t * ptr = buffer_write_ptr(&client_data->write_buffer, &max_len);
        
        int len = sprintf((char *)ptr, "%d %ld\n", i, client_data->mails[i].octets);

        buffer_write_adv(&client_data->write_buffer, len);

        bytes_written += len;
    }
}