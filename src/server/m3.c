#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "./include/m3.h"

#define _GET_BYTES(buffer, type) (*((type *) (buffer)))
#define GET_UINT8(buffer) _GET_BYTES(buffer, uint8_t)
#define GET_UINT16(buffer) _GET_BYTES(buffer, uint16_t)
#define GET_UINT32(buffer) _GET_BYTES(buffer, uint32_t)


// EXP: debemos convertir de Big Endian (que se usa en UDP) a Little Endian (para usar en C) y vice versa

void mng_buffer_to_request(uint8_t * request, mng_request * converted){
  
    size_t offset = 0;

    // EXP: al ser de solo 1 byte, no hace falta la conversion
    converted->version = GET_UINT8(request);
    offset += sizeof(converted->version);

    converted->auth_token = ntohl(GET_UINT32(request + offset));
    offset += sizeof(converted->auth_token);

    converted->op_code = GET_UINT8(request + offset);
    offset += sizeof(converted->op_code);

    converted->request_id = GET_UINT8(request + offset);
    offset += sizeof(converted->request_id);

    size_t len = ntohs(GET_UINT16(request + offset));
    offset += sizeof(converted->length);

    // EXP: me protejo en contra de que me mande mas de lo permitido
    converted->length =  len > MAX_DATA_LEN ? MAX_DATA_LEN : len;

    memcpy(converted + offset, converted->data, converted->length);
}

void mng_response_to_buffer(mng_response * response, uint8_t * converted, size_t * converted_len){

    size_t offset = 0;

    memcpy(converted + offset, &response->version, sizeof(response->version));
    offset += sizeof(response->version);

    memcpy(converted + offset, &response->status, sizeof(response->status));
    offset += sizeof(response->status);

    memcpy(converted + offset, &response->op_code, sizeof(response->op_code));
    offset += sizeof(response->op_code);

    memcpy(converted + offset, &response->request_id, sizeof(response->request_id));
    offset += sizeof(response->request_id);

    uint16_t length = htons(response->length);
    memcpy(converted + offset, &length, sizeof(response->length));
    offset += sizeof(length);

    memcpy(converted + offset, response->data, response->length);
    offset += response->length;

    *converted_len =  offset;
}

void mng_buffer_to_response(uint8_t * request, mng_response * converted) {
    size_t offset = 0;

    // EXP: al ser de solo 1 byte, no hace falta la conversion
    converted->version = GET_UINT8(request);
    offset += sizeof(converted->version);

    converted->status = GET_UINT8(request + offset);
    offset += sizeof(converted->status);

    converted->op_code = GET_UINT8(request + offset);
    offset += sizeof(converted->op_code);

    converted->request_id = GET_UINT8(request + offset);
    offset += sizeof(converted->request_id);

    uint16_t len = ntohs(GET_UINT16(request + offset));
    offset += sizeof(uint16_t);

    // EXP: me protejo en contra de que me mande mas de lo permitido
    converted->length =  len > MAX_DATA_LEN ? MAX_DATA_LEN : len;

    memcpy(converted->data, request + offset, converted->length);
}

void mng_request_to_buffer(mng_request * request, uint8_t * converted, size_t * converted_len) {
    size_t offset = 0;

    memcpy(converted + offset, &request->version, sizeof(request->version));
    offset += sizeof(request->version);

    uint32_t token = htonl(request->auth_token);    
    memcpy(converted + offset, &token, sizeof(token));
    offset += sizeof(uint32_t);

    memcpy(converted + offset, &request->op_code, sizeof(request->op_code));
    offset += sizeof(request->op_code);

    memcpy(converted + offset, &request->request_id, sizeof(request->request_id));
    offset += sizeof(request->request_id);

    uint16_t len = htons(request->length);
    memcpy(converted + offset, &len, sizeof(uint16_t));
    offset += sizeof(request->length);

    memcpy(converted + offset, &request->data, request->length);
    offset += request->length;

    *converted_len =  offset;
}

