#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

void send_request(const char *host, int port, const char *path) {
    int sock;
    struct sockaddr_in server_addr;
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Prepare the HTTP GET request
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);

    // Send the request
    send(sock, request, strlen(request), 0);

    // Read the response
    printf("Response:\n");
    while (1) {
        int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
        if (bytes_received <= 0) {
            break; // Connection closed or error
        }

        response[bytes_received] = '\0'; // Null-terminate the received data
        printf("%s", response); // Print received data
    }

    // Close the socket
    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <proxy_host> <proxy_port> <request_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *proxy_host = argv[1];
    int proxy_port = atoi(argv[2]);
    const char *request_path = argv[3];

    send_request(proxy_host, proxy_port, request_path);

    return 0;
}