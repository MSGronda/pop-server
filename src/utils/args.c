// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <stdio.h> 
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>

#include "./include/args.h"

static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}

static void
user(char *s, users_data *user) {
    char *p = strchr(s, ':');
    if(p == NULL) {
        fprintf(stderr, "password not found\n");
        exit(1);
    } else {
        *p = 0;
        p++;
        user->name = s;
        user->pass = p;
        user->sessionActive = false;
    }

}

static void
version(void) {
    fprintf(stderr, "pop3 version 0.0\n"
                    "ITBA Protocolos de Comunicación 2023/1 -- Grupo 03\n"
                    "AQUI VA LA LICENCIA\n");
}

static void
usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h               Imprime la ayuda y termina.\n"
        // "   -l <POP3 addr>   Dirección donde servirá el servidor POP3.\n"
        // "   -L <conf  addr>  Dirección donde servirá el servicio de management.\n"
        "   -p <POP3 port>   Puerto entrante conexiones POP3.\n"
        "   -P <conf port>   Puerto entrante conexiones configuracion.\n"
        "   -u <name>:<pass> Usuario y contraseña de usuario que puede usar el proxy. Hasta 10.\n"
        "   -f <path>        Path absoluto de la carpeta donde se encuentran los mails.\n"
        "   -v               Imprime información sobre la versión versión y termina.\n"
        "\n",
        progname);
    exit(1);
}

void parse_args(int argc, char * argv[], struct pop3_server_state * pop3_server_state){

    int c;
    int nusers = 0;

    while(true){

        c = getopt(argc, argv, "hl:L:p:P:u:f:v");
        if( c == -1 )
            break;
        
        switch (c) {
        case 'h':
            usage(argv[0]);
            break;
        // case 'l':

        //     break;
        // case 'L':

        //     break;
        case 'p':
            pop3_server_state->port = port(optarg);
            break;
        case 'P':
            pop3_server_state->mng_port = port(optarg);
            break;
        case 'u':
            if(nusers >= MAX_USERS) {
                    fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                    exit(1);
                } else {
                    user(optarg, &pop3_server_state->users[nusers]);
                    nusers++;
                    pop3_server_state->amount_users = nusers;
                }
            break;
        case 'f':
            pop3_server_state->folder_address = optarg;
            break;
        case 'v':
            version();
            exit(0);
            break;        
        default:
            fprintf(stderr, "unknown argument %d.\n", c);
            exit(1);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
    if (optind > argc)
    {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }
}