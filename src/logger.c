#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_LOG_ENTRIES 100
#define BUFFER_SIZE 8192

typedef struct {
  char *ip;
  char *timestamp;
  pid_t pid;
} LogEntry;

static LogEntry log_entries[MAX_LOG_ENTRIES];
static int log_index = 0;
static FILE *log_file = NULL;

void init_logger(const char *filename) {
  log_file = fopen(filename, "a");
  if (!log_file) {
    perror("Failed to open log file");
    exit(EXIT_FAILURE);
  }
}

void log_request(const char *client_ip) {
  if (log_index < MAX_LOG_ENTRIES) {
    time_t t;
    time(&t);
    char *timestamp = ctime(&t);
    timestamp[strlen(timestamp) - 1] = '\0';

    log_entries[log_index].ip = strdup(client_ip);
    log_entries[log_index].timestamp = strdup(timestamp);
    log_entries[log_index].pid = getpid();
    log_index++;

    if (log_file) {
      fprintf(log_file, "Logged Request: IP = %s, Timestamp = %s, PID = %d\n",
              client_ip, timestamp, getpid());
      fflush(log_file);
    }
  }
}

void send_logs(int client_socket) {
  char response[BUFFER_SIZE];
  int response_length = 0;

  response_length += snprintf(response + response_length, BUFFER_SIZE - response_length,
                               "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");

  for (int i = 0; i < log_index; i++) {
    response_length += snprintf(response + response_length, BUFFER_SIZE - response_length,
                                "IP: %s, Timestamp: %s, PID: %d\n",
                                log_entries[i].ip, log_entries[i].timestamp, log_entries[i].pid);
  }

  write(client_socket, response, response_length);
}

void close_logger() {
  if (log_file) fclose(log_file);
}