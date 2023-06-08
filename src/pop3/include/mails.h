#ifndef MAILS_H
#define MAILS_H


#include "../../utils/include/buffer.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// = = = = = CONSTANTES = = = = = 

#define ERROR_OPENDIR 3
#define ERROR_STAT 2
#define ERROR_ALLOC 1
#define MAILS_SUCCESS 0

// = = = = = CORREOS DEL USUARIO = = = = = 
typedef struct mail_data{
    char * name;
    size_t octets;
}mail_data;

#include "./pop3.h"     // TODO: check

unsigned int initialize_mails(client_connection_data * client_data);
void list_mails(client_connection_data * client_data);

#endif