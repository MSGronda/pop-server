
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "./include/m3.h"
#include "./include/server.h"
#include "./include/management.h"

#include "../utils/include/logger.h"
#include "../pop3/include/users.h"


// = = = = = =  DEFINES Y MACROS  = = = = = =  

#define UDP_BUFFER_SIZE 1024

// = = = = = =  ACTIONS  = = = = = =  

typedef uint8_t (*mng_action)(mng_request * request, mng_response * response);

uint8_t mng_get_bytes_sent(mng_request * request, mng_response * response){
    uint32_t sent = get_server_state()->metrics.bytes_sent;
    memcpy(response->data, &sent, sizeof(sent));
    response->length = sizeof(sent);
    return MNG_SUCCESS;
}
uint8_t mng_get_bytes_recieved(mng_request * request, mng_response * response){
    uint32_t recieved = get_server_state()->metrics.bytes_recieved;
    memcpy(response->data, &recieved, sizeof(recieved));
    response->length = sizeof(recieved);
    return MNG_SUCCESS;
}

uint8_t mng_get_total_connections(mng_request * request, mng_response * response){
    uint32_t total_connections = get_server_state()->metrics.total_connections;
    memcpy(response->data, &total_connections, sizeof(total_connections));
    response->length = sizeof(total_connections);
    return MNG_SUCCESS;
}

uint8_t mng_get_curr_connections(mng_request * request, mng_response * response){
    uint32_t curr_connections = get_server_state()->metrics.current_connections;
    memcpy(response->data, &curr_connections, sizeof(curr_connections));
    response->length = sizeof(curr_connections);
    return MNG_SUCCESS;
}

uint8_t mng_noop(mng_request * request, mng_response * response){
    response->length = 0;
    return MNG_SUCCESS;
}

uint8_t mng_add_user(mng_request * request, mng_response * response){

    bool success = add_user((char *) request->data);

    if(!success){
        return MNG_INVALID_ARGS;
    }

    return MNG_SUCCESS;
}

// WARNING: deben estar en el mismo orden que el enum
static const mng_action mng_v1_actions[] = {
    &mng_get_bytes_sent,
    &mng_get_bytes_recieved,
    &mng_get_total_connections,
    &mng_get_curr_connections,
    &mng_add_user,
    &mng_noop
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

    if(request->op_code > MNG_NOOP){
        response->status = MNG_INVALID_OP_CODE;
        return;
    }

    // EXP: devolvemos el mismo op_code que nos mando
    response->op_code = request->op_code;

    uint8_t status =  mng_v1_actions[request->op_code](request, response);

    // EXP: el status esta dictado por las actions
    response->status = status;
}

void mng_passive_handler(struct selector_key *key){
  
    // EXP: necesito guardarme el address del cliente para despues poder mandarle la respuesta.
    struct sockaddr_storage client_address;
    socklen_t client_address_len = sizeof(client_address);

    uint8_t read_buffer[UDP_BUFFER_SIZE], write_buffer[UDP_BUFFER_SIZE];
    memset(read_buffer, 0, UDP_BUFFER_SIZE);
    memset(write_buffer, 0, UDP_BUFFER_SIZE);

    mng_request request;
    mng_response response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));

    ssize_t recieved_count = recvfrom(key->fd, read_buffer, UDP_BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &client_address_len);

    if(recieved_count < 0){
        log(DEBUG, "%s", "Error recieving data from management socket (UDP)")
        return;
    }

    // EXP: tengo que convertir el buffer a un struct
    mng_buffer_to_request(read_buffer, &request);
 
    mng_handle_request(&request, &response);

    size_t write_len;

    // EXP: tengo que convertir el struct a un buffer
    // EXP: debe tambien reportar la longitud de la respuesta
    mng_response_to_buffer(&response, write_buffer, &write_len);

    sendto(key->fd, write_buffer, write_len, 0, (const struct sockaddr *)&client_address, client_address_len);
}