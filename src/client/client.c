#include "./include/client.h"

#define BUFF_SIZE 1024


int main() {
    int socket_fd;
    struct sockaddr_in server_address;
    socklen_t server_address_len = sizeof(server_address);
    const char* server_ip = "127.0.0.1";  // Replace with the target IP address
    int server_port = 25566;  // Replace with the target port number

    // Create socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    uint8_t buffer[BUFF_SIZE];
    mng_request request;
    request.version = MNG_V1;
    request.auth_token = AUTH_TOKEN;
    request.op_code = MNG_GET_BYTES_SENT;
    request.request_id = 3;
    request.length = 0;
    
    // Send UDP packet
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
        printf("data: %d\n", ntohl(data));
    }

    close(socket_fd);

    return 0;
}