#include "./include/main.h"

#define MAX_PENDING_CONNECTIONS 600
#define SELECTOR_SIZE 1024

int main(){
    char * error_msg;


    // = = = = = CONFIGURACION DEL SOCKET = = = = = =
    
    // EXP: usamos IPv6 directamente porque puede aceptar conexiones 
    // IPv4 y IPv6 al mismo tiempo.

    int socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if(socket_fd < 0){
        ERROR_CATCH("Error creating socket", finally)
    }

    // EXP: Deshabilito reportar si falla
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int));

    struct sockaddr_in6 address;
    memset(&address, 0, sizeof(address));

    // EXP: seteo la informacion del socket
    address.sin6_family = AF_INET6;
    address.sin6_port = htons(25565);
    address.sin6_addr = in6addr_any;       // any address TODO: check

    // EXP: bindeo el socket
    if (bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        ERROR_CATCH("Error binding socket", finally)
    }
    
    // EXP: listeneo (?) el socket
    if(listen(socket_fd, MAX_PENDING_CONNECTIONS) < 0){
        ERROR_CATCH("Error listening to socket", finally)
    }

    // EXP: setea fd como NON_BLOCING;
    if(selector_fd_set_nio(socket_fd) == -1){
        ERROR_CATCH("Error setting socket as non-blocking", finally)
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
    if (selector_init(&configuration) != 0){
        ERROR_CATCH("Error initializing selector library", finally)
    }

    // EXP: genero un nuevo selector. Todos los socket y otras funcionalidades se "meten" a este selector
    fd_selector selector = selector_new(SELECTOR_SIZE);
    if(selector == NULL){
        ERROR_CATCH("Error creating new selector", finally)
    }

    const struct fd_handler handlers = {
        .handle_read = &pop3_passive_handler,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    // EXP: le especifico que cuando un cliente intenta leer del socket pasivo, que ejecute el handler
    selector_status s = selector_register(selector, socket_fd, &handlers, OP_READ, NULL);
    
    if(s != SELECTOR_SUCCESS){
        ERROR_CATCH("Error registering selector", finally)
    }

    
    // EXP: loop infinito
    while(1){
        s = selector_select(selector);
        if(s != SELECTOR_SUCCESS){
            ERROR_CATCH("Error executing select", finally)
        }
    }


finally:
    fprintf(stderr, "%s\n", error_msg);
    return 0;
}