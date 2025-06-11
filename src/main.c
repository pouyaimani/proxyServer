#include "logger.h"
#include "request_handler.h"
#include "signal_handler.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s --inbound <IP:port> --outbound <hostname>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *inbound_host = strtok(argv[2], ":");
  char *inbound_port_str = strtok(NULL, ":");
  int inbound_port = atoi(inbound_port_str);
  char *outbound_host = argv[4];

  init_logger("proxy_requests.log");
  setup_signal_handler();

  int listener_socket;
  struct sockaddr_in listener_addr;

  if ((listener_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }

  memset(&listener_addr, 0, sizeof(listener_addr));
  listener_addr.sin_family = AF_INET;
  listener_addr.sin_addr.s_addr = inet_addr(inbound_host);
  listener_addr.sin_port = htons(inbound_port);

  if (bind(listener_socket, (struct sockaddr *)&listener_addr,
           sizeof(listener_addr)) < 0) {
    perror("Failed to bind socket");
    exit(EXIT_FAILURE);
  }

  if (listen(listener_socket, 10) < 0) {
    perror("Failed to listen on socket");
    exit(EXIT_FAILURE);
  }

  printf("Proxy server listening on %s:%d, forwarding to %s\n", inbound_host,
         inbound_port, outbound_host);

  while (1) {
    int client_socket = accept(listener_socket, NULL, NULL);
    if (client_socket < 0) {
      perror("Failed to accept connection");
      continue;
    }

    pid_t pid = fork();
    if (pid == 0) {
#ifdef CPU_SET
      cpu_set_t cpus;
      CPU_ZERO(&cpus);
      CPU_SET(getpid() % sysconf(_SC_NPROCESSORS_ONLN), &cpus);
      sched_setaffinity(0, sizeof(cpus), &cpus);
#endif
      close(listener_socket);
      handle_request(client_socket, outbound_host);
      exit(0);
    } else if (pid > 0) {
      close(client_socket);
    } else {
      perror("Failed to fork");
      close(client_socket);
    }
  }

  close_logger();
  close(listener_socket);
  return 0;
}
