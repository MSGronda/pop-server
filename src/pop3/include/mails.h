#ifndef MAILS_H
#define MAILS_H

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>



#include "../../utils/include/buffer.h"
#include "../../utils/include/selector.h"
#include "../../include/common.h"

// = = = = = CONSTANTES = = = = = 

typedef enum initialize_status{
    MAILS_SUCCESS,
    ERROR_ALLOC,
    ERROR_STAT,
    ERROR_DIR
}initialize_status;

#define MAX_MAILS 100

#define RETRIEVE_BUFFER_SIZE 1024

// = = = = = CORREOS DEL USUARIO = = = = = 
typedef struct mail_data{
    char * name;
    size_t octets;
    int state;                          // 0 borrado 1 activo
}mail_data;

typedef struct stuffing_parser{
    uint8_t prev;               // i - 1
    uint8_t second_prev;        // i - 2
    uint8_t third_prev;         // i - 3
}stuffing_parser;

typedef struct user_mail_info{
    mail_data mails[MAX_MAILS];  
    size_t mail_count;                  // cantidad de mails totales que tenia el usuario al hacer login
    size_t current_count;               // cantidad de mails actuales (puede haber borrado)
    size_t total_octets;                // tamano total de todos los mails

    buffer  retrive_buffer;
    uint8_t retrive_addr[RETRIEVE_BUFFER_SIZE];
    int filed_fd;
    size_t bytes_read;
    int finished_reading;
    stuffing_parser parser;
}user_mail_info;

#include "./pop3_structures.h"


unsigned int initialize_mails(user_mail_info * mail_info, char * username, char * maildir_path);
int list_mails(buffer * write_buffer, user_mail_info * mail_info, size_t * bytes_written);
int list_mail(buffer * write_buffer, user_mail_info * mail_info, char * mail_num);
void stat_mailbox(buffer * write_buffer, user_mail_info * mail_info);
void delete_mail(buffer * write_buffer, user_mail_info * mail_info, char * arg);
void restore_mail(buffer * write_buffer, user_mail_info * mail_info);
int retrieve_mail(struct selector_key *key, char * maildir);
void free_mail_info(struct selector_key *key);
int user_file_name(char ** file_name, char * username, char * maildir);

#endif