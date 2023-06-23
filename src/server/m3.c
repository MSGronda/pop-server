#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "./include/m3.h"

#define _GET_BYTES(buffer, type) (*((type *) (buffer)))
#define GET_UINT8(buffer) _GET_BYTES(buffer, uint8_t)
#define GET_UINT16(buffer) _GET_BYTES(buffer, uint16_t)
#define GET_UINT32(buffer) _GET_BYTES(buffer, uint32_t)


// EXP: debemos convertir de Big Endian (que se usa en UDP) a Little Endian (para usar en C) y vice versa

void mng_buffer_to_request(uint8_t * request, mng_request * converted) {
  
    size_t offset = 0;

    // EXP: al ser de solo 1 byte, no hace falta la conversion
    converted->version = GET_UINT8(request);
    offset += sizeof(uint8_t);

    converted->auth_token = GET_UINT32(request + offset);
    offset += sizeof(uint32_t);

    converted->op_code = GET_UINT8(request + offset);
    offset += sizeof(uint8_t);

    converted->request_id = GET_UINT8(request + offset);
    offset += sizeof(uint8_t);

    size_t len = GET_UINT16(request + offset);
    offset += sizeof(uint16_t);

    // EXP: me protejo en contra de que me mande mas de lo permitido
    converted->length =  len > MAX_DATA_LEN ? MAX_DATA_LEN : len;

    memcpy(converted->data, request + offset, converted->length);
}

void mng_response_to_buffer(mng_response * response, uint8_t * converted, size_t * converted_len) {

    size_t offset = 0;

    memcpy(converted + offset, &response->version, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(converted + offset, &response->status, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(converted + offset, &response->op_code, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(converted + offset, &response->request_id, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    uint16_t length = response->length;
    memcpy(converted + offset, &length, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(converted + offset, response->data, response->length);
    offset += response->length;

    *converted_len =  offset;
}

void mng_buffer_to_response(uint8_t * request, mng_response * converted) {
    size_t offset = 0;

    // EXP: al ser de solo 1 byte, no hace falta la conversion
    converted->version = GET_UINT8(request);
    offset += sizeof(uint8_t);

    converted->status = GET_UINT8(request + offset);
    offset += sizeof(uint8_t);

    converted->op_code = GET_UINT8(request + offset);
    offset += sizeof(uint8_t);

    converted->request_id = GET_UINT8(request + offset);
    offset += sizeof(uint8_t);

    uint16_t len = GET_UINT16(request + offset);
    offset += sizeof(uint16_t);

    // EXP: me protejo en contra de que me mande mas de lo permitido
    converted->length =  len > MAX_DATA_LEN ? MAX_DATA_LEN : len;

    memcpy(converted->data, request + offset, converted->length);
}

void mng_request_to_buffer(mng_request * request, uint8_t * converted, size_t * converted_len) {
    size_t offset = 0;

    memcpy(converted + offset, &request->version, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    uint32_t token = request->auth_token;    
    memcpy(converted + offset, &token, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(converted + offset, &request->op_code, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(converted + offset, &request->request_id, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    uint16_t len = request->length;
    memcpy(converted + offset, &len, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(converted + offset, &request->data, request->length);
    offset += request->length;

    *converted_len =  offset;
}

