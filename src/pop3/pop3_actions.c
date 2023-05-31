#include "include/pop3_actions.h"

// = = = = = ACTIONS = = = = = 

unsigned int pop3_read_action(struct selector_key *key) {
    size_t read_max;   
    uint8_t* buffer;
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: le pido a libreria de buffer, un puntero a una zona de memoria
    // EXP: me devuelve ese puntero y la cantidad de de bytes que puede leer
    buffer = buffer_write_ptr(&client_data->read_buffer, &read_max);

    // EXP: leo de forma no bloqueante
    ssize_t read_count = recv(key->fd, buffer, read_max, 0);                        // TODO: check read_count <= 0 (?)

    // EXP: avanzo el puntero de escritura en la libreria de buffers
    buffer_write_adv(&client_data->read_buffer, read_count);

    //TODO: remove, esto es una demo para ver si funca.
    while(buffer_can_read(&client_data->read_buffer)) {
        const char c = buffer_read(&client_data->read_buffer);
        printf("%c", c);
    }

    return REQ_READ_STATE;         
}
unsigned int pop3_write_action(struct selector_key *key) {
    printf("WRITE\n");
    return REQ_READ_STATE;
}