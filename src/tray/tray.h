#ifndef TRAY_H
#define TRAY_H

#ifdef __WIN32
#include <windows.h>
#include <shellapi.h>
#endif
#ifdef __linux
#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#endif

struct tray_menu;

struct tray {
  const char *icon;
  char *tooltip;
  struct tray_menu *menu;
  unsigned int id;
#ifdef __WIN32
  NOTIFYICONDATA nid;
  HWND hwnd;
  HMENU hmenu;
#endif
#ifdef __linux
  AppIndicator *indicator;
  int loop_result;
#endif
};

struct tray_menu {
  struct tray *tray;
  const char *text;
  int disabled;
  int checked;
  int checkbox;

  void (*cb)(struct tray_menu *);
  void *context;

  struct tray_menu *submenu;
};

int tray_init(struct tray *tray);
int tray_loop(struct tray *tray, int blocking);
void tray_update(struct tray *tray);
void tray_exit(struct tray *tray);

#endif /* TRAY_H */
