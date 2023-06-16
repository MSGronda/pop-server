#ifndef M3_H
#define M3_H

#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>


// EXP: el cliente debe usar este token para autenticarse
#define AUTH_TOKEN 0x368F8339

#define MAX_DATA_LEN 256 * 2 + 10 


typedef enum mng_version{
    MNG_V1 = 1
}mng_version;

typedef enum mng_opertation{
    MNG_GET_BYTES_SENT,
    MNG_GET_BYTES_RECIEVED,
    MNG_GET_TOTAL_CONNECTIONS,
    MNG_GET_CURR_CONNECTIONS,
    MNG_ADD_USER,
    MNG_NOOP,
}mng_opertation;

typedef enum mng_status_code{
    MNG_SUCCESS,
    MNG_INVALID_VERSION,
    MNG_INVALID_OP_CODE,
    MNG_INVALID_TOKEN,
    MNG_INVALID_ARGS,
}mng_status_code;


typedef struct mng_request{
    uint8_t version;
    uint32_t auth_token;
    uint8_t op_code;
    uint8_t request_id;
    uint16_t length;                    // length de la data en bytes
    uint8_t data[MAX_DATA_LEN];
}mng_request;


typedef struct mng_response{
    uint8_t version;
    uint8_t status;
    uint8_t op_code;
    uint8_t request_id;
    uint16_t length;                    // length de la data en bytes
    uint8_t data[MAX_DATA_LEN];
}mng_response;

void mng_buffer_to_request(uint8_t * request, mng_request * converted);
void mng_response_to_buffer(mng_response * response, uint8_t * converted, size_t * converted_len);
void mng_request_to_buffer(mng_request * request, uint8_t * converted, size_t * converted_len);
void mng_buffer_to_response(uint8_t * request, mng_response * converted);
#endif