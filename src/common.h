#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define BUFFER_SMALL_SIZE 256
#define ssend(sock, s, flags) (send(sock, s, strlen(s), flags))

#ifdef __WIN32
#define close(sock) closesocket(sock)
#endif

void set_blocking(int sock, bool b) {
#ifdef __WIN32
  unsigned long int mode = !b;
  ioctlsocket(sock, FIONBIO, &mode);
#else
  fcntl(sock, F_SETFL, b ? -O_NONBLOCK : O_NONBLOCK);
#endif
}

enum status {
  READY,
  WORKING,
  PROBLEM,
  DISCONNECTED
};

int string_to_status(char *s) {
  if (strcmp(s, "READY") == 0)
    return READY;
  else if (strcmp(s, "WORKING") == 0)
    return WORKING;
  else if (strcmp(s, "PROBLEM") == 0)
    return PROBLEM;
  else if (strcmp(s, "DISCONNECTED") == 0)
    return PROBLEM;
  else
    return -1;
}

#endif
