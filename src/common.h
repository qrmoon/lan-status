#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define BUFFER_SMALL_SIZE 256
#define ssend(sock, s, flags) (send(sock, s, strlen(s), flags))

#define LOOP_SLEEP 0.001

#ifdef __WIN32
#include <windows.h>
#define close(sock) closesocket(sock)
#define msleep(sec) Sleep(sec * 1000)
#else
#include <unistd.h>
#define msleep(sec) usleep(sec * 1000000)
#endif

#include "lang.h"

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

time_t ptime() {
  return time(NULL);
}

struct config {
  char addr[BUFFER_SMALL_SIZE];
  int port;
  char lang[BUFFER_SMALL_SIZE];
};

bool *load_config(struct config *config, char *path) {
  FILE *file = fopen(path, "r");

  if (file == NULL) {
    printf(LOCAL_TEXT(CANNOT_OPEN_FILE), path);
    return false;
  }

  char key[BUFFER_SMALL_SIZE];
  char value[BUFFER_SMALL_SIZE];
  char line[BUFFER_SIZE];
  while (fgets(line, BUFFER_SIZE, file)) {
    memset(key, 0, sizeof(char)*BUFFER_SMALL_SIZE);
    memset(value, 0, sizeof(char)*BUFFER_SMALL_SIZE);

    int n = sscanf(line, "%s %s", key, value);
    if (n != 2) {
      printf(LOCAL_TEXT(CONFIG_ERROR));
      fclose(file);
      return false;
    }

    if (strcmp(key, "addr") == 0)
      memcpy(config->addr, value, BUFFER_SMALL_SIZE);
    else if (strcmp(key, "port") == 0)
      config->port = atoi(value);
    else if (strcmp(key, "lang") == 0)
      memcpy(config->lang, value, BUFFER_SMALL_SIZE);
    else
      printf(LOCAL_TEXT(UNKNOWN_FIELD), key);
  }
  fclose(file);

  return true;
}

#endif
