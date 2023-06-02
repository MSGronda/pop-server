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
    ssize_t recieved_count = recv(key->fd, buffer, read_max, 0);                        // TODO: check read_count <= 0 (?)

    // EXP: avanzo el puntero de escritura en la libreria de buffers
    buffer_write_adv(&client_data->read_buffer, recieved_count);

    //TODO: remove, esto es una demo para ver si funca.
    while(buffer_can_read(&client_data->read_buffer) && buffer_can_write(&client_data->write_buffer)) {
        const char c = buffer_read(&client_data->read_buffer);
        buffer_write(&client_data->write_buffer,c);
    }

    selector_set_interest_key(key, OP_WRITE);

    return SOCKET_IO_WRITE;
}

unsigned int socket_write(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);


    // EXP: llamaron al read action sin nada en el buffer de salida, PANICO
    if(!buffer_can_read(&client_data->write_buffer)) {
        // TODO: estado de error (?)

        return SOCKET_IO_READ;
    }
    // EXP: consigo lo que puedo escribir y lo intento mandar
    // EXP: solo avanzo (en el buffer) la cantidad que realmente pude mandar
    size_t write_max;
    uint8_t * buffer = buffer_read_ptr(&client_data->write_buffer, &write_max);
    ssize_t sent_count = send(key->fd, buffer, write_max, 0);

    buffer_read_adv(&client_data->write_buffer, sent_count);

    // EXP: puede mandar todo. ahora tengo que esperar hasta que el usuario mande algo
    if(!buffer_can_read(&client_data->write_buffer)){
        selector_set_interest_key(key, OP_READ);
        return SOCKET_IO_READ;
    } 
    // EXP: no se mando todo, quiero que intente devuelta
    return SOCKET_IO_WRITE;             
}