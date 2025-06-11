#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <sys/types.h>

void init_logger(const char *filename);
void log_request(const char *client_ip);
void send_logs(int client_socket);
void close_logger();

#endif // LOGGER_H