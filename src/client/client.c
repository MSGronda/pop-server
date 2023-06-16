#include "./include/client.h"

#define MAX_BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];
    const char* serverIP = "127.0.0.1";  // Replace with the target IP address
    int serverPort = 25566;  // Replace with the target port number

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    // Send UDP packet
    const char* message = "Hello, UDP!";
    ssize_t numBytesSent = sendto(sockfd, message, strlen(message), 0,
                                  (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (numBytesSent < 0) {
        perror("Failed to send packet");
        exit(EXIT_FAILURE);
    }

    printf("UDP packet sent successfully.\n");

    close(sockfd);

    return 0;
}