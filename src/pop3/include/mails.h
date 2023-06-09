#ifndef MAILS_H
#define MAILS_H


#include "../../utils/include/buffer.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <limits.h>

typedef struct running_command running_command;

// = = = = = CONSTANTES = = = = = 

#define ERROR_OPENDIR 3
#define ERROR_STAT 2
#define ERROR_ALLOC 1
#define MAILS_SUCCESS 0

#define MAX_MAILS 100

// = = = = = CORREOS DEL USUARIO = = = = = 
typedef struct mail_data{
    char * name;
    size_t octets;
    int state;
}mail_data;

typedef struct user_mail_info{
    mail_data mails[MAX_MAILS];  
    size_t mail_count;
    size_t current_count;
    size_t total_octets;                // tamano total de todos los mails
}user_mail_info;

#include "./pop3.h"     // TODO: check

unsigned int initialize_mails(user_mail_info * mail_info, char * username);
int list_mails(buffer * write_buffer, user_mail_info * mail_info, running_command * command);
int list_mail(buffer * write_buffer, user_mail_info * mail_info, char * mail_num);
void stat_mailbox(buffer * write_buffer, user_mail_info * mail_info);
void delete_mail(buffer * write_buffer, user_mail_info * mail_info, char * arg);
void restore_mail(buffer * write_buffer, user_mail_info * mail_info);

#endif