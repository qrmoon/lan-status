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

#define SA struct sockaddr

#define TIMEOUT 2.0
#define DEFAULT_CLIENT_NAME "klient"

typedef struct {
  int sock;
  int status;
  clock_t last_ping;
  char name[BUFFER_SMALL_SIZE];
  char addr[BUFFER_SMALL_SIZE];
  int tray_id;
  struct tray tray;
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


int main() {
#ifdef __WIN32
  FreeConsole();
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    return -1;
  }
#endif
  int ret = 0;

  char icons[4][32] = {
    "icons/ready.ico",
    "icons/working.ico",
    "icons/problem.ico",
    "icons/disconnected.ico"
  };

  FILE *file = fopen("peers", "r");
  if (file != NULL) {
    char addr[BUFFER_SMALL_SIZE];
    char name[BUFFER_SMALL_SIZE];
    memset(addr, 0, BUFFER_SMALL_SIZE);
    memset(name, 0, BUFFER_SMALL_SIZE);
    while (fscanf(file, "%s %s", addr, name) == 2) {
      if (peers_len == 0)
        peers = malloc(sizeof(peer_t));
      else
        peers = realloc(peers, sizeof(peer_t)*(peers_len+1));
      memset(peers+peers_len, 0, sizeof(peer_t));
      memcpy(peers[peers_len].name, name, BUFFER_SMALL_SIZE);
      memcpy(peers[peers_len].addr, addr, BUFFER_SMALL_SIZE);
      peers[peers_len].sock = -1;
      peers[peers_len].status = DISCONNECTED;
      peers_len++;
    }
    fclose(file);
  } else {
    printf("Cannot open `peers`\n");
    return 1;
  }

  char buffers[peers_len][BUFFER_SIZE];
  memset(buffers, 0, peers_len*BUFFER_SIZE);

  struct tray main_tray = {
    .icon = "icons/main.ico",
    .tooltip = "Wszyscy",
    .menu = (struct tray_menu[]) {
      { .text = "Gotowy", .cb = ready_all_cb },
      { .text = "W trakcie", .cb = working_all_cb },
      { .text = "Problem", .cb = problem_all_cb },
      { .text = "-" },
      { .text = "Zamknij", .cb = quit_cb },
      { .text = NULL }
    }
  };
  tray_init(&main_tray);

  for (int i=0; i<peers_len; i++) {
    peers[i].tray.icon = icons[DISCONNECTED];
    peers[i].tray.tooltip = peers[i].name;

    peers[i].tray.menu = malloc(sizeof(struct tray_menu)*6);
    memset(peers[i].tray.menu, 0, sizeof(struct tray_menu)*6);
    peers[i].tray.menu[0].text = peers[i].name;
    peers[i].tray.menu[1].text = "-";

    peers[i].tray.menu[2].text = "Gotowy";
    peers[i].tray.menu[2].context = &peers[i];
    peers[i].tray.menu[2].cb = ready_cb;

    peers[i].tray.menu[3].text = "W trakcie";
    peers[i].tray.menu[3].context = &peers[i];
    peers[i].tray.menu[3].cb = working_cb;

    peers[i].tray.menu[4].text = "Problem";
    peers[i].tray.menu[4].context = &peers[i];
    peers[i].tray.menu[4].cb = problem_cb;

    peers[i].tray.menu[5].text = NULL;
    if (tray_init(&peers[i].tray) < 0) {
      ret = 1;
      printf("Cannot create tray icon\n");
      goto cleanup;
    }
  }

  int sock;
  struct sockaddr_in server_addr;
  char addr[BUFFER_SMALL_SIZE];
  int port;
  file = fopen("host", "r");
  if (file == NULL) return 6;
  fscanf(file, "%s %d", addr, &port);
  fclose(file);

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    ret = 1;
    printf("Cannot open socket\n");
    goto cleanup;
  }
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
    printf("Invalid address\n");
    ret = 1;
    goto cleanup;
  }

  if (bind(sock, (SA*)&server_addr, sizeof(server_addr)) < 0) {
    ret = 1;
    printf("Cannot bind\n");
    goto cleanup;
  }

  if (listen(sock, peers_len*2) < 0) {
    ret = 1;
    printf("Cannot listen\n");
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
          peers[i].last_ping = clock();
          matched = true;
          printf(
            "%s [%d]: connected %s\n",
            peers[i].name,
            i,
            addr
          );
          ssend(peers[i].sock, "READY\n", 0);
          peers[i].status = READY;
        }
      }
      if (!matched) {
        printf(
          "[%s]: unknown address or already connected\n",
          addr
        );
        close(connfd);
      }
    }

    if (tray_loop(&main_tray, 0) != 0) break;
    for (int i=0; i<peers_len; i++) {
      if (tray_loop(&peers[i].tray, 0) != 0)
        break;
      if (peers[i].sock == -1) continue;

      if ((float)(clock()-peers[i].last_ping)/CLOCKS_PER_SEC > TIMEOUT) {
        close(peers[i].sock);
        peers[i].sock = -1;
        peers[i].status = DISCONNECTED;
        peers[i].tray.icon = icons[DISCONNECTED];
        tray_update(&peers[i].tray);
        printf("%s [%d]: disconnected\n", peers[i].name, i);
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
              peers[i].last_ping = clock();
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

      if (peers[i].tray.icon != icons[peers[i].status]) {
        peers[i].tray.icon = icons[peers[i].status];
        tray_update(&peers[i].tray);
      }
    }
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
