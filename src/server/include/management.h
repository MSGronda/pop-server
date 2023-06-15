#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <stdint.h>

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
    MNG_INVALID_ARGS,
}mng_status_code;


typedef struct mng_request{
    mng_version version;
    uint32_t auth_token;
    mng_opertation op_code;
    uint8_t request_id;
    uint16_t length;
    int8_t data[100];   // TODO change
}mng_request;


typedef struct mng_response{
    mng_version version;
    mng_status_code status;
    mng_opertation op_code;
    uint8_t request_id;
    uint16_t length;
    int8_t data[100];   // TODO change
}mng_response;

#endif