// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/**
 * buffer.c - buffer con acceso directo (útil para I/O) que mantiene
 *            mantiene puntero de lectura y de escritura.
 */

#include <string.h>
#include <assert.h>



#include "./include/buffer.h"

inline void
buffer_reset(buffer *b) {
    b->read  = b->data;
    b->write = b->data;
}

void
buffer_init(buffer *b, const size_t n, uint8_t *data) {
    b->data = data;
    buffer_reset(b);
    b->limit = b->data + n;
}


inline bool
buffer_can_write(buffer *b) {
    return b->limit - b->write > 0;
}

inline uint8_t *
buffer_write_ptr(buffer *b, size_t *nbyte) {
    assert(b->write <= b->limit);
    *nbyte = b->limit - b->write;
    return b->write;
}

inline bool
buffer_can_read(buffer *b) {
    return b->write - b->read > 0;
}

inline uint8_t *
buffer_read_ptr(buffer *b, size_t *nbyte) {
    assert(b->read <= b->write);
    *nbyte = b->write - b->read;
    return b->read;
}

inline void
buffer_write_adv(buffer *b, const ssize_t bytes) {
    if(bytes > -1) {
        b->write += (size_t) bytes;
        assert(b->write <= b->limit);
    }
}

inline void
buffer_read_adv(buffer *b, const ssize_t bytes) {
    if(bytes > -1) {
        b->read += (size_t) bytes;
        assert(b->read <= b->write);

        if(b->read == b->write) {
            // compactacion poco costosa
            buffer_compact(b);
        }
    }
}

inline uint8_t
buffer_read(buffer *b) {
    uint8_t ret;
    if(buffer_can_read(b)) {
        ret = *b->read;
        buffer_read_adv(b, 1);
    } else {
        ret = 0;
    }
    return ret;
}

inline void
buffer_write(buffer *b, uint8_t c) {
    if(buffer_can_write(b)) {
        *b->write = c;
        buffer_write_adv(b, 1);
    }
}

void
buffer_compact(buffer *b) {
    if(b->data == b->read) {
        // nada por hacer
    } else if(b->read == b->write) {
        b->read  = b->data;
        b->write = b->data;
    } else {
        const size_t n = b->write - b->read;
        memmove(b->data, b->read, n);
        b->read  = b->data;
        b->write = b->data + n;
    }
}

void
buffer_write_chunk(buffer * b, char * msg, size_t len, unsigned * msg_pos, bool * finished) {
    size_t max_write;
    uint8_t * write_dir = buffer_write_ptr(b,&max_write);

    // EXP: escribo o el maximo que me acepta el buffer (dejando parte del mensaje afuera)
    // EXP: o escribo todo el mensaje
    size_t write_amount = len - *msg_pos > max_write ? max_write : len - *msg_pos;
    
    memcpy(write_dir, msg + *msg_pos, write_amount);
    buffer_write_adv(b, write_amount);

    *msg_pos += write_amount;

    if(*msg_pos < len) {
        *finished = false;
    }
    else{
        *finished = true;
        *msg_pos = 0;
    }
}

size_t
buffer_write_n(buffer * b, char * msg, size_t len) {
    size_t max_len;
    uint8_t * ptr =  buffer_write_ptr(b, &max_len);

    size_t write_len = max_len > len ? len : max_len;

    memcpy(ptr, msg, write_len);
    buffer_write_adv(b, write_len);

    return write_len;
}
