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

#define TIMEOUT 2.0
#define DEFAULT_CLIENT_NAME "klient"
#define MAX_ICON_INDEX 24

typedef struct {
  int sock;
  int status;
  time_t last_ping;
  char name[BUFFER_SMALL_SIZE];
  char addr[BUFFER_SMALL_SIZE];
  int tray_id;
  struct tray tray;
  int index;
} peer_t;
static peer_t *peers;
static int peers_len = 0;

static void ready_cb(struct tray_menu *item) {
  peer_t *peer = (peer_t*)item->context;
  peer->status = READY;
  ssend(peer->sock, "READY\n", 0);
  tray_update(item->tray);
}

static void working_cb(struct tray_menu *item) {
  peer_t *peer = (peer_t*)item->context;
  peer->status = WORKING;
  ssend(peer->sock, "WORKING\n", 0);
  tray_update(item->tray);
}

static void problem_cb(struct tray_menu *item) {
  peer_t *peer = (peer_t*)item->context;
  peer->status = PROBLEM;
  ssend(peer->sock, "PROBLEM\n", 0);
  tray_update(item->tray);
}

static void ready_all_cb(struct tray_menu *item) {
  for (int i=0; i<peers_len; i++) {
    if (peers[i].status == DISCONNECTED) continue;
    peers[i].status = READY;
    ssend(peers[i].sock, "READY\n", 0);
    tray_update(&peers[i].tray);
  }
}

static void working_all_cb(struct tray_menu *item) {
  for (int i=0; i<peers_len; i++) {
    if (peers[i].status == DISCONNECTED) continue;
    peers[i].status = WORKING;
    ssend(peers[i].sock, "WORKING\n", 0);
    tray_update(&peers[i].tray);
  }
}

static void problem_all_cb(struct tray_menu *item) {
  for (int i=0; i<peers_len; i++) {
    if (peers[i].status == DISCONNECTED) continue;
    peers[i].status = PROBLEM;
    ssend(peers[i].sock, "PROBLEM\n", 0);
    tray_update(&peers[i].tray);
  }
}

static void quit_cb(struct tray_menu *item) {
  tray_exit(item->tray);
}


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

  int ret = 0;

  char icons[4][32] = {
    "icons/ready.ico",
    "icons/working.ico",
    "icons/problem.ico",
    "icons/disconnected.ico"
  };

  // TODO: check for files up to a limit and expand array
  char numbered_icons[4][MAX_ICON_INDEX][128];
  memset(numbered_icons, 0, sizeof(char)*4*MAX_ICON_INDEX*128);
  for (int i = 1; i <= MAX_ICON_INDEX; i++) {
    char buff[BUFFER_SMALL_SIZE];

    sprintf(buff, "icons/numbered/ready_%d.ico", i);
    strcpy(numbered_icons[0][i-1], buff);

    sprintf(buff, "icons/numbered/working_%d.ico", i);
    strcpy(numbered_icons[1][i-1], buff);

    sprintf(buff, "icons/numbered/problem_%d.ico", i);
    strcpy(numbered_icons[2][i-1], buff);

    sprintf(buff, "icons/numbered/disconnected_%d.ico", i);
    strcpy(numbered_icons[3][i-1], buff);
  }

  FILE *file = fopen("peers", "r");
  if (file != NULL) {
    char addr[BUFFER_SMALL_SIZE];
    char name[BUFFER_SMALL_SIZE];
    int index;
    memset(addr, 0, BUFFER_SMALL_SIZE);
    memset(name, 0, BUFFER_SMALL_SIZE);
    char line[BUFFER_SMALL_SIZE];
    while (fgets(line, BUFFER_SMALL_SIZE, file)) {
      int n = sscanf(line, "%s %s %d", addr, name, &index);
      if (peers_len == 0)
        peers = malloc(sizeof(peer_t));
      else
        peers = realloc(peers, sizeof(peer_t)*(peers_len+1));
      memset(peers+peers_len, 0, sizeof(peer_t));
      memcpy(peers[peers_len].name, name, BUFFER_SMALL_SIZE);
      memcpy(peers[peers_len].addr, addr, BUFFER_SMALL_SIZE);
      peers[peers_len].sock = -1;
      peers[peers_len].status = DISCONNECTED;
      if (n >= 3 && index <= MAX_ICON_INDEX)
        peers[peers_len].index = index;
      else
        peers[peers_len].index = 0;
      peers_len++;
    }
    fclose(file);
  } else {
    printf(LOCAL_TEXT(CANNOT_OPEN_FILE), "peers");
    return 1;
  }

  char buffers[peers_len][BUFFER_SIZE];
  memset(buffers, 0, peers_len*BUFFER_SIZE);

  struct tray main_tray = {
    .icon = "icons/main.ico",
    .tooltip = LOCAL_TEXT(TRAY_EVERYBODY),
    .menu = (struct tray_menu[]) {
      { .text = LOCAL_TEXT(STATUS_READY), .cb = ready_all_cb },
      { .text = LOCAL_TEXT(STATUS_WORKING), .cb = working_all_cb },
      { .text = LOCAL_TEXT(STATUS_PROBLEM), .cb = problem_all_cb },
      { .text = "-" },
      { .text = LOCAL_TEXT(TRAY_CLOSE), .cb = quit_cb },
      { .text = NULL }
    }
  };
  tray_init(&main_tray);

  for (int i=0; i<peers_len; i++) {
    if (peers[i].index == 0)
      peers[i].tray.icon = icons[DISCONNECTED];
    else
      peers[i].tray.icon = numbered_icons[DISCONNECTED][peers[i].index-1];
    peers[i].tray.tooltip = peers[i].name;

    peers[i].tray.menu = malloc(sizeof(struct tray_menu)*6);
    memset(peers[i].tray.menu, 0, sizeof(struct tray_menu)*6);
    peers[i].tray.menu[0].text = peers[i].name;
    peers[i].tray.menu[1].text = "-";

    peers[i].tray.menu[2].text = LOCAL_TEXT(STATUS_READY);
    peers[i].tray.menu[2].context = &peers[i];
    peers[i].tray.menu[2].cb = ready_cb;

    peers[i].tray.menu[3].text = LOCAL_TEXT(STATUS_WORKING);
    peers[i].tray.menu[3].context = &peers[i];
    peers[i].tray.menu[3].cb = working_cb;

    peers[i].tray.menu[4].text = LOCAL_TEXT(STATUS_PROBLEM);
    peers[i].tray.menu[4].context = &peers[i];
    peers[i].tray.menu[4].cb = problem_cb;

    peers[i].tray.menu[5].text = NULL;
    if (tray_init(&peers[i].tray) < 0) {
      ret = 1;
      printf(LOCAL_TEXT(CANNOT_CREATE_TRAY_ICON));
      goto cleanup;
    }
  }

  int sock;
  struct sockaddr_in server_addr;
  char *addr = config.addr;
  int port = config.port;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    ret = 1;
    printf(LOCAL_TEXT(CANNOT_OPEN_SOCKET));
    goto cleanup;
  }
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
    printf(LOCAL_TEXT(INVALID_ADDRESS));
    ret = 1;
    goto cleanup;
  }

  if (bind(sock, (SA*)&server_addr, sizeof(server_addr)) < 0) {
    ret = 1;
    printf(LOCAL_TEXT(CANNOT_BIND_ADDRESS));
    goto cleanup;
  }

  if (listen(sock, peers_len*2) < 0) {
    ret = 1;
    printf(LOCAL_TEXT(CANNOT_LISTEN_ON_ADDRESS));
    goto cleanup;
  }

  set_blocking(sock, false);

  while (true) {
    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    int connfd = accept(sock, (SA*)&peer, &len);
    if (connfd >= 0) {
      set_blocking(connfd, false);
      char addr[BUFFER_SMALL_SIZE];
      inet_ntop(AF_INET, &peer.sin_addr, addr, INET_ADDRSTRLEN);
      bool matched = false;
      for (int i=0; i<peers_len; i++) {
        if (strcmp(addr, peers[i].addr) == 0
            && peers[i].status == DISCONNECTED) {
          peers[i].sock = connfd;
          peers[i].last_ping = ptime();
          matched = true;
          printf(
            "%s [%d]: %s %s\n",
            peers[i].name,
            i,
            LOCAL_TEXT(PEER_CONNECTED),
            addr
          );
          ssend(peers[i].sock, "READY\n", 0);
          peers[i].status = READY;
        }
      }
      if (!matched) {
        printf(
          "[%s]: %s\n",
          addr,
          LOCAL_TEXT(PEER_UNKNOWN_ADDRESS_OR_ALREADY_CONNECTED)
        );
        close(connfd);
      }
    }

    if (tray_loop(&main_tray, 0) != 0) break;
    for (int i=0; i<peers_len; i++) {
      if (tray_loop(&peers[i].tray, 0) != 0)
        break;
      if (peers[i].sock == -1) continue;

      if (ptime()-peers[i].last_ping > TIMEOUT) {
        close(peers[i].sock);
        peers[i].sock = -1;
        peers[i].status = DISCONNECTED;
        if (peers[i].index == 0)
          peers[i].tray.icon = icons[DISCONNECTED];
        else
          peers[i].tray.icon = numbered_icons[DISCONNECTED][peers[i].index-1];
        tray_update(&peers[i].tray);
        printf("%s [%d]: %s\n", peers[i].name, i, LOCAL_TEXT(PEER_DISCONNECTED));
        continue;
      }

      char buffer[BUFFER_SIZE];
      memset(buffer, 0, BUFFER_SIZE);
      int n;

      if ((n = recvfrom(peers[i].sock, buffer, BUFFER_SIZE, 0, NULL, 0)) > 0) {
        for (int j=0; j<n; j++) {
          if (buffer[j] == '\n') {
            int status = string_to_status(buffers[i]);
            if (status >= 0) {
              peers[i].status = status;
              printf("%s [%d] -> %s\n", peers[i].name, i, buffers[i]);
            } else if (strcmp(buffers[i], "PING") == 0) {
              peers[i].last_ping = ptime();
            } else {
              puts(buffers[i]);
            }
            memset(buffers[i], 0, BUFFER_SIZE);
          } else {
            if (strlen(buffers[i]) == BUFFER_SIZE) {
              memset(buffers[i], 0, BUFFER_SIZE);
            }
            buffers[i][strlen(buffers[i])] = buffer[j];
          }
        }
      }

      if (peers[i].index == 0) {
        if (peers[i].tray.icon != icons[peers[i].status]) {
          peers[i].tray.icon = icons[peers[i].status];
          tray_update(&peers[i].tray);
        }
      } else {
       if (peers[i].tray.icon != numbered_icons[peers[i].status][peers[i].index-1]) {
          peers[i].tray.icon = numbered_icons[peers[i].status][peers[i].index-1];
          tray_update(&peers[i].tray);
        }
      }
    }
    msleep(LOOP_SLEEP);
  }

cleanup:
#ifdef __WIN32
  WSACleanup();
#endif
  if (sock) close(sock);
  for (int i=0; i<peers_len; i++) {
    if (peers[i].tray.menu) {
      free(peers[i].tray.menu);
    }
  }
  free(peers);
  return ret;
}
