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

#define BUFF_SIZE 1024

static void help();


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

    int loop = 1;
    int id = 0;

    while(loop) {
        int operation;
        memset(read_buffer, 0, 100);
        fgets(read_buffer, 100, stdin);

        read_buffer[strcspn(read_buffer, "\r\n")] = 0;
        if(strcmp(read_buffer,"help") == 0) {
            help();
            continue;
        } else if(strcmp(read_buffer,"btsent") == 0) {
            operation = 0;
        } else if(strcmp(read_buffer,"btrec") == 0) {
            operation = 1;
        } else if(strcmp(read_buffer,"currusers") == 0) {
            operation = 3;
        } else if(strcmp(read_buffer,"hisusers") == 0) {
            operation = 2;
        } else if(strcmp(read_buffer,"adduser") == 0) {
            operation = 4;
            continue;
        } else if(strcmp(read_buffer,"noop") == 0) {
            operation = 5;
        } else if(strcmp(read_buffer,"quit") == 0) {
            break;
        } else {
            printf("Error, no matching command found\n");
            continue;
        }



        uint8_t buffer[BUFF_SIZE];
        mng_request request;
        request.version = MNG_V1;
        request.auth_token = AUTH_TOKEN;
        request.op_code = operation;
        request.request_id = id++;
        request.length = 0;

        size_t request_len;
        mng_request_to_buffer(&request, buffer, &request_len);

        ssize_t numBytesSent = sendto(socket_fd, buffer, request_len, 0, (struct sockaddr*)&server_address, server_address_len);
        if (numBytesSent < 0) {
            perror("Failed to send packet");
            exit(EXIT_FAILURE);
        }

        // EXP: reseteo el buffer
        memset(buffer, 0, BUFF_SIZE);

        ssize_t recieved_count = recvfrom(socket_fd, buffer, BUFF_SIZE, 0, (struct sockaddr *)&server_address, &server_address_len);

        if(recieved_count < 0){
            perror("Failed to recieve packet");
            exit(EXIT_FAILURE);
        }

        mng_response response;
        mng_buffer_to_response(buffer, &response);

        printf("version: %d\n", response.version);
        printf("status: %d\n", response.status);
        printf("op_code: %d\n", response.op_code);
        printf("request_id: %d\n", response.request_id);
        printf("length: %d\n", response.length);

        if(response.op_code == MNG_GET_BYTES_SENT || response.op_code == MNG_GET_BYTES_RECIEVED ||
        response.op_code == MNG_GET_TOTAL_CONNECTIONS || response.op_code == MNG_GET_CURR_CONNECTIONS){
            uint32_t data = *((uint32_t *) response.data);
            printf("data: %d\n", data);
        }

    }
    
    close(socket_fd);

    return 0;
}

static void help() {
    printf( "+-----------------------------------------+\n"
            "|Possible commands for client admin in m3 |\n"
            "+-----------------------------------------+\n"
            "|help: prints help table                  |\n"
            "|btsent: prints bytes sent                |\n"
            "|btrec: prints bytes received             |\n"
            "|currusers: prints amount ofcurrent users |\n"
            "|hisusers: prints historic amount of users|\n"
            "|adduser <user> <pass>: adds a new user   |\n"
            "|noop: noops                              |\n"
            "|quit: quits session                      |\n"
            "+-----------------------------------------+\n");
}