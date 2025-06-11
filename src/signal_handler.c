#include "signal_handler.h"
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

static void sigchld_handler(int s) {
  (void)s;
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

void setup_signal_handler() {
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);
}
