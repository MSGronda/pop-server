// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "include/socket_io_actions.h"

// = = = = = SOCKET I/O = = = = = 

unsigned int socket_read(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    size_t read_max;   
    uint8_t * buffer;
    
    // EXP: le pido a libreria de buffer, un puntero a una zona de memoria
    // EXP: me devuelve ese puntero y la cantidad de de bytes que puede leer
    buffer = buffer_write_ptr(&client_data->read_buffer, &read_max);

    // EXP: leo de forma no bloqueante
    ssize_t recieved_count = recv(key->fd, buffer, read_max, 0);

    if(recieved_count == -1) {
        return SOCKET_ERROR;
    }
    if(recieved_count == 0) {
        return SOCKET_DONE;
    }

    // EXP: avanzo el puntero de escritura en la libreria de buffers
    buffer_write_adv(&client_data->read_buffer, recieved_count);

    return SOCKET_READ_OK;
}

unsigned int socket_write(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: consigo lo que puedo escribir y lo intento mandar
    // EXP: solo avanzo (en el buffer) la cantidad que realmente pude mandar
    size_t write_max;
    uint8_t * buffer = buffer_read_ptr(&client_data->write_buffer, &write_max);
    ssize_t sent_count = send(key->fd, buffer, write_max, MSG_NOSIGNAL);

    if(sent_count == -1){
        return SOCKET_ERROR;
    }

    buffer_read_adv(&client_data->write_buffer, sent_count);

    return SOCKET_WRITE_OK;       
}

