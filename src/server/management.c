#include "./include/management.h"

// = = = = = =  DEFINES Y MACROS  = = = = = =  

#define UDP_BUFFER_SIZE 1024

// = = = = = =  ACTIONS  = = = = = =  

typedef void (*mng_action)(mng_response * response);

void mng_get_bytes_sent(mng_response * response){
    
}
void mng_get_bytes_recieved(mng_response * response){
    
}

// WARNING: deben estar en el mismo orden que el enum
static const mng_action mng_v1_actions[] = {
    &mng_get_bytes_sent,
    &mng_get_bytes_recieved,
};


void mng_handle_request(mng_request * request, mng_response * response){

    memset(response, 0, sizeof(mng_response));

    // EXP: digo al cliente que version se usa. Si fuese a mandar un request con una version
    // EXP: no soportada por el servidor, recibira MNG_INVALID_VERSION y este puede fijarse que 
    // EXP: version debe usar en la respuesta del serivdor. TODAS las versiones tienen la version
    // EXP: en el primer byte.
    response->version = MNG_V1;

    // EXP: le devuelvo al cliente el mismo request_id que me mando
    // EXP: permite al cliente distinguir entre distintos paquetes que mando
    response->request_id = request->request_id;

    // EXP: el servidor maneja UNA sola version del protocolo de monitoreo, si no es la adecuada, no lo acepta
    if(request->version != MNG_V1){
        response->status = MNG_INVALID_VERSION;
        return;
    }

    if(request->auth_token != AUTH_TOKEN){
        response->status = MNG_INVALID_TOKEN;
        return;
    }

    if(request->op_code > OP_NOOP){
        response->status = MNG_INVALID_OP_CODE;
        return;
    }

    mng_v1_actions[request->op_code](response);
}

void mng_passive_handler(struct selector_key *key){
  
    // EXP: necesito guardarme el address del cliente para despues poder mandarle la respuesta.
    struct sockaddr_storage client_address;
    socklen_t client_address_len = sizeof(client_address);

    uint8_t read_buffer[UDP_BUFFER_SIZE], write_buffer[UDP_BUFFER_SIZE];
    size_t write_len;
    mng_request request;
    mng_response response;
    memset(read_buffer, 0, UDP_BUFFER_SIZE);
    memset(write_buffer, 0, UDP_BUFFER_SIZE);


    ssize_t recieved_count = recvfrom(key->fd, read_buffer, UDP_BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &client_address_len);

    if(recieved_count < 0){
        // TODO: handle error
        return;
    }

    // EXP: tengo que convertir el buffer a un struct
    mng_buffer_to_request(read_buffer, &request);
 
    mng_handle_request(&request, &response);

    // EXP: tengo que convertir el struct a un buffer
    // EXP: debe tambien reportar la longitud de la respuesta
    mng_response_to_buffer(&response, write_buffer, &write_len);

    sendto(key->fd, write_buffer, write_len, 0, (const struct sockaddr *)&client_address, client_address_len);
}