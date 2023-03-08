#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#ifdef __WIN32
#include <w32api.h>
#define WINVER                  WindowsVista
#define _WIN32_WINDOWS          WindowsVista
#define _WIN32_WINNT            WindowsVista
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif

#include "tray/tray.h"
#include "common.h"
#include "locale.h"
#include "locales/all.h"

#define SA struct sockaddr

#define DEFAULT_PORT 5050
#define PING_INTERVAL 0.5

int sconnect(struct sockaddr_in *server_addr, char **err) {
  int sock;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    *err = NULL;
    return -1;
  }

  if (connect(sock, (SA*)server_addr, sizeof(*server_addr)) < 0) {
    if (err) *err = LOCAL_TEXT(CANNOT_CONNECT_TO_SERVER);
    close(sock);
    return -1;
  }

  set_blocking(sock, false);

  if (ssend(sock, "PING\n", 0) < 0) {
    if (err) *err = LOCAL_TEXT(CANNOT_SEND_MESSAGE);
    close(sock);
    return -1;
  }

  return sock;
}

static int status = READY;
static bool status_changed = false;

static struct tray tray;

static void ready_cb(struct tray_menu *item) {
  status = READY;
  status_changed = true;
  tray_update(&tray);
}

static void working_cb(struct tray_menu *item) {
  status = WORKING;
  status_changed = true;
  tray_update(&tray);
}

static void problem_cb(struct tray_menu *item) {
  status = PROBLEM;
  status_changed = true;
  tray_update(&tray);
}

static void quit_cb(struct tray_menu *item) {
  tray_exit(item->tray);
}

static struct tray tray = {
  .menu = (struct tray_menu[]) {
    { .text = NULL, .cb = ready_cb },
    { .text = NULL, .cb = working_cb },
    { .text = NULL, .cb = problem_cb },
    { .text = "-" },
    { .text = NULL, .cb = quit_cb },
    { .text = NULL }
  }
};


int main(int argc, char *argv[]) {
#ifdef __WIN32
  bool console = false;
#else
  bool console = true;
#endif

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-console") == 0)
      console = true;
  }

#ifdef __WIN32
  if (!console) FreeConsole();
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    return -1;
  }
#endif
  struct config config;
  load_config(&config, "config");

  if (!set_locale_str(config.lang)) {
    printf(LOCAL_TEXT(UNKNOWN_LOCALE), config.lang);
  }

  tray.menu[0].text = LOCAL_TEXT(STATUS_READY);
  tray.menu[1].text = LOCAL_TEXT(STATUS_WORKING);
  tray.menu[2].text = LOCAL_TEXT(STATUS_PROBLEM);
  tray.menu[4].text = LOCAL_TEXT(TRAY_CLOSE);

  char *addr = config.addr;
  int port = config.port;

  struct sockaddr_in server_addr;

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
    printf(LOCAL_TEXT(INVALID_ADDRESS));
    return 1;
  }

  char *err = NULL;
  int sock = sconnect(&server_addr, &err);
  if (sock < 0) {
    if (err) printf("%s", err);
    close(sock);
  }

  if (sock >= 0) {
    if (ssend(sock, "PING\n", 0) < 0) {
      printf(LOCAL_TEXT(CANNOT_SEND_MESSAGE));
      close(sock);
    }
  }

  char icons[4][32] = {
    "icons/ready.ico",
    "icons/working.ico",
    "icons/problem.ico",
    "icons/disconnected.ico"
  };

  tray.icon = icons[status];
  if (tray_init(&tray) < 0) {
    close(sock);
    return 1;
  }

  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  time_t last_ping = 0;
  int last_status = READY;
  while (true) {
    if (status_changed) {
      switch (status) {
        case 0:
          ssend(sock, "READY\n", 0);
          break;
        case 1:
          ssend(sock, "WORKING\n", 0);
          break;
        case 2:
          ssend(sock, "PROBLEM\n", 0);
          break;
      }
      status_changed = false;
    }
    if (ptime()-last_ping >= PING_INTERVAL) {
      last_ping = ptime();
      if (sock < 0 || ssend(sock, "PING\n", 0) < 0) {
        close(sock);

        if (status != DISCONNECTED)
          last_status = status;

        char *err = NULL;
        sock = sconnect(&server_addr, &err);

        if (sock < 0)
          status = DISCONNECTED;
        else
          status = last_status;
        tray_update(&tray);

        if (err) printf("%s", err);
      }
    }
    char buff[BUFFER_SIZE];
    memset(buff, 0, BUFFER_SIZE);
    int n;
    if ((n = recvfrom(sock, buff, BUFFER_SIZE, 0, NULL, 0)) > 0) {
      for (int i=0; i<n; i++) {
        if (buff[i] == '\n') {
          int st = string_to_status(buffer);
          if (st >= 0) {
            status = st;
            tray_update(&tray);
          } else if (strcmp(buffer, "PING") == 0)
            ;
          else
            puts(buffer);
          memset(buffer, 0, BUFFER_SIZE);
        } else {
          if (strlen(buffer) == BUFFER_SIZE) {
            memset(buffer, 0, BUFFER_SIZE);
          }
          buffer[strlen(buffer)] = buff[i];
        }
      }
    }

    if (tray_loop(&tray, 0) == 0) {
      if (tray.icon != icons[status]) {
        tray.icon = icons[status];
        tray_update(&tray);
      }
    } else {
      break;
    }

    msleep(LOOP_SLEEP);
  }

  return 0;
}
