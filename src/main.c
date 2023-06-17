// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/signal.h>

#include "./include/common.h"

#include "./server/include/server.h" 
#include "../server/include/management.h"

#include "./pop3/include/pop3.h"
#include "./pop3/include/users.h"

#include "./utils/include/selector.h"
#include "./utils/include/buffer.h"
#include "./utils/include/logger.h"
#include "./utils/include/args.h"
#include "./include/main.h"

#define MAX_PENDING_CONNECTIONS 500
#define SELECTOR_SIZE 1024

#define SOCKET_FAILED -1

int generate_socket(int type, int protocol, int port, fd_selector selector, fd_handler * handlers, char ** msg){

    // EXP: usamos IPv6 directamente porque puede aceptar conexiones IPv4 y IPv6 al mismo tiempo.
    
    int socket_fd = socket(AF_INET6, type, protocol);

    if(socket_fd < 0) {
        *msg = "Error creating passive socket";
        return SOCKET_FAILED;
    }

    // EXP: Deshabilito reportar si falla
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int));

    struct sockaddr_in6 address;
    memset(&address, 0, sizeof(address));

    // EXP: seteo la informacion del socket
    address.sin6_family = AF_INET6;
    address.sin6_port = htons(port);
    address.sin6_addr = in6addr_any;                     // any address TODO: check

    // EXP: bindeo el socket
    if (bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        *msg = "Error binding passive socket";
        return SOCKET_FAILED;
    }

    if(type == SOCK_STREAM){
        // EXP: pongo para escuchar el socket (solo en TCP)
        if(listen(socket_fd, MAX_PENDING_CONNECTIONS) < 0) {
            *msg = "Error binding passive socket";
            return SOCKET_FAILED;
        }
    }

    // EXP: setea fd como NON_BLOCKING;
    if(selector_fd_set_nio(socket_fd) == -1) {
        *msg = "Error setting socket for passive socket as non-blocking";
        return SOCKET_FAILED;
    }

     // EXP: le especifico que cuando un cliente intenta leer del socket pasivo, que ejecute el handler
    selector_status select_status = selector_register(selector, socket_fd, handlers, OP_READ, NULL);
    
    if(select_status != SELECTOR_SUCCESS) {
        *msg = "Error registering selector for passive socket";
        return SOCKET_FAILED;
    }
    return socket_fd;
}


int main(int argc, char * argv[]) {
    int ret;
    char * error_msg = NULL;
    int pop3_socket_fd = -1, mng_socket_fd = -1;
    fd_selector selector = NULL;
    selector_status init_status;
    selector_status select_status;
    bool server_state_initialized;

    setLogLevel(DEBUG);

    log(INFO, "%s", "Server initializing...")

    server_state_initialized = initialize_server_state();

    struct pop3_server_state * server_state = get_server_state();

    if(server_state_initialized == false) {
        ERROR_CATCH("Initializing server state", finally)
    }

    parse_args(argc, argv, server_state);

    // = = = = = CONFIGURACION DEL SELECTOR = = = = = =

    // EXP: inicializo libreria de selectores
    struct selector_init configuration = {
        .signal = SIGALRM,
        .select_timeout = {
                .tv_sec  = 10,
                .tv_nsec = 0,
        }
    };
    init_status = selector_init(&configuration);
    if (init_status != 0) {
        ERROR_CATCH("Error initializing selector library", finally)
    }

    // EXP: genero un nuevo selector. Todos los socket y otras funcionalidades se "meten" a este selector
    selector = selector_new(SELECTOR_SIZE);
    if(selector == NULL) {
        ERROR_CATCH("Error creating new selector for passive socket", finally)
    }

    // = = = = = CONFIGURACION DEL SOCKET PARA POP3 = = = = = =
    struct fd_handler pop3_handlers = {
        .handle_read = &pop3_passive_handler,
        .handle_write = NULL,
        .handle_close = NULL,
        .handle_block = NULL
    };
    
    pop3_socket_fd = generate_socket(SOCK_STREAM, IPPROTO_TCP, server_state->port, selector, &pop3_handlers, &error_msg);

    if(pop3_socket_fd == SOCKET_FAILED){
        goto finally;
    }
    else{
        log(DEBUG, "Pop3 passive socket created successfully with fd: %d", pop3_socket_fd);
    }

    // = = = = = CONFIGURACION DEL SOCKET PARA MANAGEMENT = = = = = =
    struct fd_handler mng_handlers = {
        .handle_read = &mng_passive_handler,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    mng_socket_fd = generate_socket(SOCK_DGRAM, 0, server_state->mng_port, selector, &mng_handlers, &error_msg);

    if(mng_socket_fd == SOCKET_FAILED) {
        goto finally;
    }
    else{
        log(DEBUG, "Management passive socket created successfully with fd: %d", mng_socket_fd);
    }

    log(INFO, "%s", "Server now accepting connection requests")

    // = = = = = LOOP INFINITO = = = = = =
    while(server_state->running) {
        select_status = selector_select(selector);
        if(select_status != SELECTOR_SUCCESS) {
            ERROR_CATCH("Error executing select for passive socket", finally)
        }
    }

finally:
    ret = error_msg == NULL ? 1 : 0;

    if(error_msg != NULL){
        log(ERROR,"%s", error_msg);
    }
    else{
        log(INFO, "%s", "Server finished execution successfully")
    }

    if(selector != NULL){
        selector_destroy(selector);
    }
    if(init_status != SELECTOR_SUCCESS){
        selector_close();
    }
    if(pop3_socket_fd > 0){
        close(pop3_socket_fd);
    }
    if(mng_socket_fd > 0){
        close(mng_socket_fd);
    }

    if(server_state_initialized != false){
        free_server_resources();
    }

    return ret;
}