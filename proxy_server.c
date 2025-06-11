#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 8192
#define LOG_FILE "proxy_requests.log"
#define MAX_LOG_ENTRIES 100

typedef struct {
  char *ip;
  char *timestamp;
  pid_t pid;
} LogEntry;

LogEntry log_entries[MAX_LOG_ENTRIES];
int log_index = 0;
FILE *log_file = NULL;

// Function to log requests to the file
void log_request(const char *client_ip) {
  if (log_index < MAX_LOG_ENTRIES) {
    time_t t;
    time(&t);
    char *timestamp = ctime(&t);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline

    // Store the log entry
    log_entries[log_index].ip = strdup(client_ip);
    log_entries[log_index].timestamp = strdup(timestamp);
    log_entries[log_index].pid = getpid();
    log_index++;

    // Write log entry to file
    if (log_file) {
      fprintf(log_file, "Logged Request: IP = %s, Timestamp = %s, PID = %d\n",
              client_ip, timestamp, getpid());
      fflush(log_file); // Flush to ensure it's written immediately
    }
  }
}

void send_logs(int client_socket) {
  char response[BUFFER_SIZE];
  int response_length = 0;

  response_length +=
      snprintf(response + response_length, BUFFER_SIZE - response_length,
               "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");

  for (int i = 0; i < log_index; i++) {
    response_length +=
        snprintf(response + response_length, BUFFER_SIZE - response_length,
                 "IP: %s, Timestamp: %s, PID: %d\n", log_entries[i].ip,
                 log_entries[i].timestamp, log_entries[i].pid);
  }
  write(client_socket, response, response_length);
}

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

  // Check if the request path is for logs
  if (strstr(buffer, "/.svc/collect_logs")) {
    log_request(client_ip);   // Log the request before sending log response
    send_logs(client_socket); // Send the logs
    close(client_socket);
    return;
  }

  // Log the request information
  log_request(client_ip);

  // Connect to the outbound server
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

void sigchld_handler(int s) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s --inbound <IP:port> --outbound <hostname>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  char *inbound_host = strtok(argv[2], ":");
  char *inbound_port_str = strtok(NULL, ":");
  int inbound_port = atoi(inbound_port_str);
  char *outbound_host = argv[4];

  // Open log file for appending
  log_file = fopen(LOG_FILE, "a");
  if (!log_file) {
    perror("Failed to open log file");
    exit(EXIT_FAILURE);
  }

  int listener_socket;
  struct sockaddr_in listener_addr;

  if ((listener_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to create socket");
    fclose(log_file);
    exit(EXIT_FAILURE);
  }

  memset(&listener_addr, 0, sizeof(listener_addr));
  listener_addr.sin_family = AF_INET;
  listener_addr.sin_addr.s_addr = inet_addr(inbound_host);
  listener_addr.sin_port = htons(inbound_port);

  if (bind(listener_socket, (struct sockaddr *)&listener_addr,
           sizeof(listener_addr)) < 0) {
    perror("Failed to bind socket");
    fclose(log_file);
    exit(EXIT_FAILURE);
  }

  if (listen(listener_socket, 10) < 0) {
    perror("Failed to listen on socket");
    fclose(log_file);
    exit(EXIT_FAILURE);
  }

  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  /*By setting this flag to SA_RESTART, 
  it means that certain system calls that are interrupted by this signal
  will be restarted automatically instead of returning an error.
  This is particularly useful to prevent errors when reading or writing data,
  thus enhancing the robustness of the program.*/
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);

  printf("Proxy server listening on %s:%d, forwarding to %s\n", inbound_host,
         inbound_port, outbound_host);

  while (1) {
    int client_socket = accept(listener_socket, NULL, NULL);
    if (client_socket < 0) {
      perror("Failed to accept connection");
      continue;
    }
    pid_t pid = fork();
    perror("-------");

    if (pid == 0) {
      // In child process
      close(listener_socket);
      // Set CPU affinity: dynamically assigned (for simplicity, just use round-robin)
#ifdef CPU_SET
      cpu_set_t cpus;
      CPU_ZERO(&cpus);
      CPU_SET(log_index % sysconf(_SC_NPROCESSORS_ONLN), &cpus);
      sched_setaffinity(0, sizeof(cpus), &cpus);
#endif
      handle_request(client_socket, outbound_host);
      exit(0);
    } else if (pid > 0) {
      // Parent process
      close(client_socket);
    } else {
      perror("Failed to fork");
      close(client_socket);
    }
  }

  // Clean up - close log file on exit (this part may never be reached unless
  // the server stops)
  fclose(log_file);
  close(listener_socket);
  return 0;
}