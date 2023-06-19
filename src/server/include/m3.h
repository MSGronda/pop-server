#ifndef M3_H
#define M3_H

#include <stdint.h>

// EXP: el cliente debe usar este token para autenticarse
// EXP: este es el default. se deberia cambiar usando el comando:  export M3_AUTH_TOKEN=...
#define DEFAULT_AUTH_TOKEN 0x368F8339

#define MAX_DATA_LEN 256 * 2 + 10 


typedef enum mng_version{
    MNG_V1 = 1
}mng_version;

typedef enum mng_opertation{
    MNG_GET_BYTES_SENT = 0,
    MNG_GET_BYTES_RECIEVED = 1,
    MNG_GET_TOTAL_CONNECTIONS = 2,
    MNG_GET_CURR_CONNECTIONS = 3,
    MNG_ADD_USER = 4,
    MNG_NOOP = 5,
}mng_opertation;

typedef enum mng_status_code{
    MNG_SUCCESS = 0,
    MNG_INVALID_VERSION = 1,
    MNG_INVALID_OP_CODE = 2,
    MNG_INVALID_TOKEN = 3,
    MNG_INVALID_ARGS = 4,
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