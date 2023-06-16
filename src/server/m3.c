#include "./include/m3.h"

#define _GET_BYTES(buffer, type) (*((type *) buffer))
#define GET_UINT8(buffer) _GET_BYTES(buffer, uint8_t)
#define GET_UINT16(buffer) _GET_BYTES(buffer, uint16_t)
#define GET_UINT32(buffer) _GET_BYTES(buffer, uint32_t)


void mng_buffer_to_request(uint8_t * request, mng_request * converted){
    // EXP: debemos convertir de Big Endian (que se usa en UDP) a Little Endian (para usar en C)
    
    size_t offset = 0;

    // EXP: al ser de solo 1 byte, no hace falta la conversion
    converted->version = GET_UINT8(request);
    offset += sizeof(uint8_t);

    converted->auth_token = ntohl(GET_UINT32(request + offset));
    offset += sizeof(uint32_t);

    converted->op_code = GET_UINT8(request);
    offset += sizeof(uint8_t);

    converted->request_id = GET_UINT8(request);
    offset += sizeof(uint8_t);

    converted->length = ntohs(GET_UINT16(request + offset));
    offset += sizeof(uint16_t);

    // TODO: check!!!
    memcpy(converted->data, request + offset, MAX_DATA_LEN);
}

void mng_response_to_buffer(mng_response * response, uint8_t * converted, size_t * converted_len){
    // EXP: y aca debemos convertir de little endian a big endian

    size_t offset = 0;

    memcpy(converted + offset, &response->version, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(converted + offset, &response->status, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(converted + offset, &response->request_id, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    uint16_t length = htons(response->length);
    memcpy(converted + offset, &length, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // TODO: check!!!
    memcpy(converted + offset, response->data, response->length);
    offset += response->length;

    *converted_len =  offset;
}