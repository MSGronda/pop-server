// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "./include/main.h"

#define MAX_PENDING_CONNECTIONS 500
#define SELECTOR_SIZE 1024


int main(int argc, char * argv[]) {
    char * error_msg;
    int socket_fd = 1, mng_socket_fd = 1;
    fd_selector selector = NULL;
    selector_status init_status;
    selector_status select_status;
    bool server_state_initialized;

    setLogLevel(DEBUG);

    server_state_initialized = initialize_server_state();

    struct pop3_server_state * server_state = get_server_state();

    if(server_state_initialized == false) {
        ERROR_CATCH("Initializing server state", error_finally)
    }

    parse_args(argc, argv, server_state);

    // = = = = = CONFIGURACION DEL SOCKET PARA POP3 = = = = = =
    
    // EXP: usamos IPv6 directamente porque puede aceptar conexiones 
    // IPv4 y IPv6 al mismo tiempo.

    socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if(socket_fd < 0) {
        ERROR_CATCH("Error creating passive socket", error_finally)
    }

    // EXP: Deshabilito reportar si falla
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int));

    struct sockaddr_in6 address;
    memset(&address, 0, sizeof(address));

    // EXP: seteo la informacion del socket
    address.sin6_family = AF_INET6;
    address.sin6_port = htons(server_state->port);
    address.sin6_addr = in6addr_any;                     // any address TODO: check

    // EXP: bindeo el socket
    if (bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        ERROR_CATCH("Error binding passive socket", error_finally)
    }
    
    // EXP: listeneo (?) el socket
    if(listen(socket_fd, MAX_PENDING_CONNECTIONS) < 0) {
        ERROR_CATCH("Error listening to passive socket", error_finally)
    }

    // EXP: setea fd como NON_BLOCING;
    if(selector_fd_set_nio(socket_fd) == -1) {
        ERROR_CATCH("Error setting socket for passive socket as non-blocking", error_finally)
    }

    // = = = = = CONFIGURACION DEL SELECTOR = = = = = =

    // EXP: inicializo libreria de selectores
    const struct selector_init configuration = {
        .signal = SIGALRM,
        .select_timeout = {
                .tv_sec  = 10,
                .tv_nsec = 0,
        }
    };
    init_status = selector_init(&configuration);
    if (init_status != 0) {
        ERROR_CATCH("Error initializing selector library", error_finally)
    }

    // EXP: genero un nuevo selector. Todos los socket y otras funcionalidades se "meten" a este selector
    selector = selector_new(SELECTOR_SIZE);
    if(selector == NULL) {
        ERROR_CATCH("Error creating new selector for passive socket", error_finally)
    }

    const struct fd_handler pop3_handlers = {
        .handle_read = &pop3_passive_handler,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    // EXP: le especifico que cuando un cliente intenta leer del socket pasivo, que ejecute el handler
    select_status = selector_register(selector, socket_fd, &pop3_handlers, OP_READ, NULL);
    
    log(INFO, "%s","Passive socket created successfully")

    if(select_status != SELECTOR_SUCCESS) {
        ERROR_CATCH("Error registering selector for passive socket", error_finally)
    }



    // = = = = = CONFIGURACION DEL SOCKET PARA MANAGEMENT = = = = = =
    
    mng_socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);

    if(mng_socket_fd < 0) {
        ERROR_CATCH("Error creating passive socket", error_finally)
    }

    // EXP: Deshabilito reportar si falla
    setsockopt(mng_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int));

    struct sockaddr_in6 mngmnt_address;
    memset(&mngmnt_address, 0, sizeof(mngmnt_address));

    // EXP: seteo la informacion del socket
    mngmnt_address.sin6_family = AF_INET6;
    mngmnt_address.sin6_port = htons(server_state->mng_port);
    mngmnt_address.sin6_addr = in6addr_any;                     // any address TODO: check

    // EXP: bindeo el socket
    if (bind(mng_socket_fd, (struct sockaddr *) &mngmnt_address, sizeof(mngmnt_address)) < 0) {
        ERROR_CATCH("Error binding passive socket", error_finally)
    }
    
    // EXP: setea fd como NON_BLOCING;
    if(selector_fd_set_nio(mng_socket_fd) == -1) {
        ERROR_CATCH("Error setting socket for passive socket as non-blocking", error_finally)
    }

    const struct fd_handler mng_handlers = {
        .handle_read = &mng_passive_handler,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    // EXP: le especifico que cuando un cliente intenta leer del socket pasivo, que ejecute el handler
    select_status = selector_register(selector, mng_socket_fd, &mng_handlers, OP_READ, NULL);
    
    log(INFO, "%s","Passive socket created successfully")

    if(select_status != SELECTOR_SUCCESS) {
        ERROR_CATCH("Error registering selector for passive socket", error_finally)
    }


    // = = = = = LOOP INFINITO = = = = = =
    
    while(server_state->running) {
        select_status = selector_select(selector);
        if(select_status != SELECTOR_SUCCESS) {
            ERROR_CATCH("Error executing select for passive socket", error_finally)
        }
    }
    
    //TODO: liberar recursos de usuarios
    return 0;

error_finally:
    log(ERROR,"%s", error_msg)

    if(server_state_initialized != false){
        destroy_server_state();
    }
    if(socket_fd > 0){
        close(socket_fd);
    }
    if(selector != NULL){
        selector_destroy(selector);
    }
    if(init_status != SELECTOR_SUCCESS){
        selector_close();
    }
    return 1;
}