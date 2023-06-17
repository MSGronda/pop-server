#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "../server/include/m3.h"
#include "./include/client.h"
#include <ctype.h>

#define BUFF_SIZE 1024

static void help();
static void handle_request(int operation, char * param, int * id, int connection, int socket_fd, 
                            struct sockaddr_in server_address, struct sockaddr_in6 server_address6, socklen_t server_address_len );


int main(int argc, char * argv[]) {

    if(argc != 3) {
        printf("Use: ./client.out <server-address> <server-port>\n");
        exit(EXIT_FAILURE);
    }

    int socket_fd;
    struct sockaddr_in server_address;
    struct sockaddr_in6 server_address6;
    socklen_t server_address_len = sizeof(server_address);

    //const char* server_ip = "127.0.0.1";  // Replace with the target IP address
    //int server_port = 25566;  // Replace with the target port number

    const char * server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int connection;

    

    // Set up server address
    memset(&server_address, 0, sizeof(server_address));
    memset(&server_address6, 0, sizeof(server_address6));

    // Get port
    if((server_port = htons(atoi(argv[2]))) <= 0) {
        perror("Invalid port\n");
        exit(EXIT_FAILURE);
    }

    
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr.s_addr) > 0) {
        server_address.sin_family = AF_INET;
        server_address.sin_port = server_port;
        connection = 4;
    } else if (inet_pton(AF_INET6, server_ip, &server_address6.sin6_addr) > 0) {
        server_address6.sin6_family = AF_INET6;
        server_address6.sin6_port = server_port;
        connection = 6;
    }
    else {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    // Create socket
    int connection_type = connection == 4 ? AF_INET : AF_INET6;
    socket_fd = socket(connection_type, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Failed to create socket\n");
        exit(EXIT_FAILURE);
    }

    char read_buffer[100];
    char * param;

    int loop = 1;
    int id = 0;

    while(loop) {
        printf("> ");
        int operation;
        memset(read_buffer, 0, 100);
        fgets(read_buffer, 100, stdin);

        read_buffer[strcspn(read_buffer, "\r\n")] = 0;
        if(strcmp(read_buffer,"help") == 0) {
            help();
            continue;
        } else if(strcmp(read_buffer,"btsent") == 0) {
            operation = MNG_GET_BYTES_SENT;
        } else if(strcmp(read_buffer,"btrec") == 0) {
            operation = MNG_GET_BYTES_RECIEVED;
        } else if(strcmp(read_buffer,"currusers") == 0) {
            operation = MNG_GET_CURR_CONNECTIONS;
        } else if(strcmp(read_buffer,"hisusers") == 0) {
            operation = MNG_GET_TOTAL_CONNECTIONS;
        } else if(strncmp(read_buffer,"adduser ", strlen("adduser ")) == 0) {
            char * inBetween = strchr(read_buffer, ':');

            if( inBetween == NULL){
                printf("Error, invalid parameters\n");
                continue;
            }
               
            param = read_buffer + strlen("adduser ");
            if( isspace(*param) ){
                printf("Error, this command doesn't ignore blanks\n");
                continue;
            }
            char * aux = param;
            while(*aux && !isspace(*aux)){
                aux++;
            }
            if( *aux){
                *aux = '\0';
            }
            operation = MNG_ADD_USER;
        } else if(strcmp(read_buffer,"noop") == 0) {
            operation = MNG_NOOP;
        } else if(strcmp(read_buffer,"quit") == 0) {
            break;
        } else {
            printf("Error, no matching command found\n");
            continue;
        }
        if( operation != MNG_ADD_USER)
            handle_request(operation, NULL, &id, connection, socket_fd, server_address, server_address6, server_address_len);
        else    
            handle_request(operation, param, &id, connection, socket_fd, server_address, server_address6, server_address_len);

    }
    
    close(socket_fd);

    return 0;
}

static void help() {
    printf( "+-------------------------------------------+\n"
            "|Possible commands for client admin in m3   |\n"
            "+-------------------------------------------+\n"
            "|`help`: prints help table                  |\n"
            "|`btsent`: prints bytes sent                |\n"
            "|`btrec`: prints bytes received             |\n"
            "|`currusers`: prints amount ofcurrent users |\n"
            "|`hisusers`: prints historic amount of users|\n"
            "|`adduser <user>:<pass>`: adds a new user   |\n"
            "|`noop`: noops                              |\n"
            "|`quit`: quits session                      |\n"
            "+-------------------------------------------+\n");
}

static void handle_request(int operation, char * param, int * id, int connection, int socket_fd, 
                            struct sockaddr_in server_address, struct sockaddr_in6 server_address6, socklen_t server_address_len ){
    uint8_t buffer[BUFF_SIZE];
    mng_request request;
    request.version = MNG_V1;
    request.auth_token = AUTH_TOKEN;
    request.op_code = operation;
    request.request_id = *id;
    request.length = 0;

    if( operation == MNG_ADD_USER){
        strcpy((char *)request.data, param);
        request.length = strlen((char *)request.data);
    }

    size_t request_len;
    mng_request_to_buffer(&request, buffer, &request_len);

    ssize_t numBytesSent;
    if(connection == IPV_4) {
        numBytesSent = sendto(socket_fd, buffer, request_len, 0, (struct sockaddr*)&server_address, server_address_len);
    } else {
        numBytesSent = sendto(socket_fd, buffer, request_len, 0, (struct sockaddr*)&server_address6, server_address_len);
    }
    if (numBytesSent < 0) {
        perror("Failed to send packet");
        exit(EXIT_FAILURE);
    }

    // EXP: reseteo el buffer
    memset(buffer, 0, BUFF_SIZE);

    ssize_t recieved_count;
    if(connection == 4) {
        recieved_count = recvfrom(socket_fd, buffer, BUFF_SIZE, 0, (struct sockaddr *)&server_address, &server_address_len);
    } else {
        recieved_count = recvfrom(socket_fd, buffer, BUFF_SIZE, 0, (struct sockaddr *)&server_address6, &server_address_len);
    }

    if(recieved_count < 0){
        perror("Failed to recieve packet");
        exit(EXIT_FAILURE);
    }

    mng_response response;
    mng_buffer_to_response(buffer, &response);

    
    switch(response.status) {
        case MNG_SUCCESS:
            (*id)++;
            break;
        case MNG_INVALID_VERSION:
            printf("Error, invalid version of response. Try again.\n");
            return;
            break;
        case MNG_INVALID_OP_CODE:
            printf("Error, invalid operation on response. Try again.\n");
            return;
            break;
        case MNG_INVALID_TOKEN:
            printf("Error, invalid token on response. Try again.\n");
            return;
            break;
        case MNG_INVALID_ARGS:
            printf("Error, invalid arguments. Try again.\n");
            return;
            break;
        default:
            printf("Unexpected error occurred. Try again.\n");
            return;
            break;
    }
    
    // printf("version: %d\n", response.version);
    // printf("status: %d\n", response.status);
    // printf("op_code: %d\n", response.op_code);
    // printf("request_id: %d\n", response.request_id);
    // printf("length: %d\n", response.length);

    switch (response.op_code)
    {
    case MNG_GET_BYTES_SENT:
        printf("Bytes sent: ");
        break;
    case MNG_GET_BYTES_RECIEVED:
        printf("Bytes recieved: ");
        break;
    case MNG_GET_TOTAL_CONNECTIONS:
        printf("Total historic connections: ");
        break;
    case MNG_GET_CURR_CONNECTIONS:
        printf("Total current connections: ");
        break;
    case MNG_ADD_USER:
        printf("User succesfully added\n");
        break;
    case MNG_NOOP:
        printf("noop\n");
        break;
    default:
        break;
    }

    if(response.op_code == MNG_GET_BYTES_SENT || response.op_code == MNG_GET_BYTES_RECIEVED ||
    response.op_code == MNG_GET_TOTAL_CONNECTIONS || response.op_code == MNG_GET_CURR_CONNECTIONS){
        uint32_t data = *((uint32_t *) response.data);
        printf("%d\n", data);
    }
}