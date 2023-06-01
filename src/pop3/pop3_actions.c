#include "include/pop3_actions.h"

// = = = = = GREETING = = = = = 

void greeting_arrival(const unsigned state, struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: copio el HELLO ahora y saco logica de greeting y todo eso de las actions
    // TODO: CHECK!!!!

    char * hello_msg = "+OK pop3-server ready\n";

    // WARNING: asumo que hay suficiente espacio en el buffer para escribir todo el mensaje
    for(int i=0; hello_msg[i]!=0; i++){
        buffer_write(&client_data->write_buffer, hello_msg[i]);
    }

}

unsigned int greeting_write(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);

    // EXP: envio greeting al cliente
    size_t write_max;
    uint8_t * buffer = buffer_read_ptr(&client_data->write_buffer, &write_max);
    ssize_t sent_count = send(key->fd, buffer, write_max, 0);
    buffer_read_adv(&client_data->write_buffer, sent_count);

    if(!buffer_can_read(&client_data->write_buffer)){
        
        // EXP: desde ahora en mas, el espero hasta que el cliente me mande para desperarme
        selector_set_interest_key(key, OP_READ);
        return COMMAND_READ_STATE;
    } 

    // EXP: no se mando todo, quiero que intente devuelta
    return COMMAND_WRITE_STATE;     
}


// = = = = = COMMAND = = = = = 

unsigned int command_read(struct selector_key *key) {
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
    while(buffer_can_read(&client_data->read_buffer)) {
        const char c = buffer_read(&client_data->read_buffer);
        printf("%c", c);
    }

    return COMMAND_READ_STATE;
}

unsigned int command_write(struct selector_key *key) {
    client_connection_data * client_data = ATTACHMENT(key);


    // EXP: llamaron al read action sin nada en el buffer de salida, PANICO
    if(!buffer_can_read(&client_data->write_buffer)) {
        // TODO: estado de error (?)

        return COMMAND_READ_STATE;
    }

    // EXP: consigo lo que puedo escribir y lo intento mandar
    // EXP: solo avanzo (en el buffer) la cantidad que realmente pude mandar
    size_t write_max;
    uint8_t * buffer = buffer_read_ptr(&client_data->write_buffer, &write_max);
    ssize_t sent_count = send(key->fd, buffer, write_max, 0);
    buffer_read_adv(&client_data->write_buffer, sent_count);


    if(!buffer_can_read(&client_data->write_buffer)) 
        return COMMAND_READ_STATE;

    // EXP: no se mando todo, quiero que intente devuelta
    return COMMAND_WRITE_STATE;             
}