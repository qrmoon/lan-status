#include <stdbool.h>
#include <windows.h>
#include <shellapi.h>
#include "tray.h"

#define WM_TRAY_CALLBACK_MESSAGE (WM_USER + 1)
#define WC_TRAY_CLASS_NAME "TRAY"

WNDCLASSEX wc;

struct tray *curtray;
int last_id = 1000;

static LRESULT CALLBACK _tray_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                       LPARAM lparam) {
  switch (msg) {
  case WM_CLOSE:
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_TRAY_CALLBACK_MESSAGE:
    if (lparam == WM_LBUTTONUP || lparam == WM_RBUTTONUP) {
      POINT p;
      GetCursorPos(&p);
      SetForegroundWindow(hwnd);
      WORD cmd = TrackPopupMenu(curtray->hmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON |
                                                TPM_RETURNCMD | TPM_NONOTIFY,
                                p.x, p.y, 0, hwnd, NULL);
      SendMessage(hwnd, WM_COMMAND, cmd, 0);
      return 0;
    }
    break;
  case WM_COMMAND:
    if (wparam >= curtray->id) {
      MENUITEMINFO item = {
          .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_ID | MIIM_DATA,
      };
      if (GetMenuItemInfo(curtray->hmenu, wparam, FALSE, &item)) {
        struct tray_menu *menu = (struct tray_menu *)item.dwItemData;
        if (menu != NULL && menu->cb != NULL) {
          menu->cb(menu);
        }
      }
      return 0;
    }
    break;
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

static HMENU _tray_menu(struct tray_menu *m, UINT *id) {
  HMENU hmenu = CreatePopupMenu();
  for (; m != NULL && m->text != NULL; m++, (*id)++) {
    if (strcmp(m->text, "-") == 0) {
      InsertMenu(hmenu, *id, MF_SEPARATOR, TRUE, "");
    } else {
      MENUITEMINFO item;
      memset(&item, 0, sizeof(item));
      item.cbSize = sizeof(MENUITEMINFO);
      item.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
      item.fType = 0;
      item.fState = 0;
      if (m->submenu != NULL) {
        item.fMask = item.fMask | MIIM_SUBMENU;
        item.hSubMenu = _tray_menu(m->submenu, id);
      }
      if (m->disabled) {
        item.fState |= MFS_DISABLED;
      }
      if (m->checked) {
        item.fState |= MFS_CHECKED;
      }
      item.wID = *id;
      item.dwTypeData = (LPSTR)m->text;
      item.dwItemData = (ULONG_PTR)m;

      InsertMenuItem(hmenu, *id, TRUE, &item);
    }
  }
  return hmenu;
}

int tray_init(struct tray *tray) {
  static bool wc_inited = false;
  if (!wc_inited) {
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = _tray_wnd_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WC_TRAY_CLASS_NAME;
    if (!RegisterClassEx(&wc)) {
      return -1;
    }
    wc_inited = true;
  }

  tray->hwnd = CreateWindowEx(0, WC_TRAY_CLASS_NAME, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  if (tray->hwnd == NULL) {
    return -1;
  }
  UpdateWindow(tray->hwnd);

  memset(&tray->nid, 0, sizeof(tray->nid));
  tray->nid.cbSize = sizeof(NOTIFYICONDATA);
  tray->nid.hWnd = tray->hwnd;
  tray->nid.uID = 0;
  tray->nid.uFlags = NIF_ICON | NIF_MESSAGE;
  tray->nid.uCallbackMessage = WM_TRAY_CALLBACK_MESSAGE;
  Shell_NotifyIcon(NIM_ADD, &tray->nid);

  tray->id = last_id += 100;

  tray_update(tray);
  return 0;
}

int tray_loop(struct tray *tray, int blocking) {
  curtray = tray;
  MSG msg;
  if (blocking) {
    GetMessage(&msg, tray->hwnd, 0, 0);
  } else {
    PeekMessage(&msg, tray->hwnd, 0, 0, PM_REMOVE);
  }
  if (msg.message == WM_QUIT) {
    return -1;
  }
  TranslateMessage(&msg);
  DispatchMessage(&msg);
  return 0;
}

void tray_update(struct tray *tray) {
  for (int i=0; tray->menu[i].text != NULL; i++)
    tray->menu[i].tray = tray;

  HMENU prevmenu = tray->hmenu;
  int id = tray->id;
  tray->hmenu = _tray_menu(tray->menu, &id);
  SendMessage(tray->hwnd, WM_INITMENUPOPUP, (WPARAM)tray->hmenu, 0);
  HICON icon;
  ExtractIconEx(tray->icon, 0, NULL, &icon, 1);
  if (tray->nid.hIcon) {
    DestroyIcon(tray->nid.hIcon);
  }
  tray->nid.hIcon = icon;
  if(tray->tooltip != 0 && strlen(tray->tooltip) > 0) {
    strncpy(tray->nid.szTip, tray->tooltip, sizeof(tray->nid.szTip));
    tray->nid.uFlags |= NIF_TIP;
  }
  Shell_NotifyIcon(NIM_MODIFY, &tray->nid);

  if (prevmenu != NULL) {
    DestroyMenu(prevmenu);
  }
}

void tray_exit(struct tray *tray) {
  Shell_NotifyIcon(NIM_DELETE, &tray->nid);
  if (tray->nid.hIcon != 0) {
    DestroyIcon(tray->nid.hIcon);
  }
  if (tray->hmenu != 0) {
    DestroyMenu(tray->hmenu);
  }
  PostQuitMessage(0);
  UnregisterClass(WC_TRAY_CLASS_NAME, GetModuleHandle(NULL));
}
