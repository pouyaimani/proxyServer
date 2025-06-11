#include "request_handler.h"
#include "logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

void handle_request(int client_socket, const char *outbound_host) {
  char buffer[BUFFER_SIZE];
  int server_socket;
  struct sockaddr_in server_addr;

  ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
  if (bytes_read < 0) {
    perror("Failed to read from client");
    close(client_socket);
    return;
  }
  buffer[bytes_read] = '\0';

  char client_ip[INET_ADDRSTRLEN];
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  getpeername(client_socket, (struct sockaddr *)&addr, &addr_len);
  inet_ntop(AF_INET, &addr.sin_addr, client_ip, sizeof(client_ip));

  if (strstr(buffer, "/.svc/collect_logs")) {
    log_request(client_ip);
    send_logs(client_socket);
    close(client_socket);
    return;
  }

  log_request(client_ip);

  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to create socket");
    close(client_socket);
    return;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(80);
  inet_pton(AF_INET, outbound_host, &server_addr.sin_addr);

  if (connect(server_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("Failed to connect to outbound server");
    close(client_socket);
    close(server_socket);
    return;
  }

  write(server_socket, buffer, bytes_read);

  while ((bytes_read = read(server_socket, buffer, sizeof(buffer))) > 0) {
    write(client_socket, buffer, bytes_read);
  }

  close(server_socket);
  close(client_socket);
}