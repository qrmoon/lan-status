#ifndef en_US_H
#define en_US_H

#include "locale.h"

char *locale_en_US[] = {
  "Cannot open `%s`\n",
  "Cannot open socket\n",

  "Cannot connect to server\n",
  "Cannot send message\n",

  "Cannot bind address\n",
  "Cannot listen on address\n",
  "Invalid address\n",

  "Cannot create tray icon\n",

  "Everybody",
  "Close",

  "connected",
  "disconnected",
  "unknown address or already connected",

  "Ready",
  "Working",
  "Problem",

  "Unknown locale `%s`\n",
  "Error in config file\n",
  "Unknown field `%s`\n"
};

char *local_string_en_US(int i) {
  return locale_en_US[i];
}

void set_locale_en_US() {
  local_string_func = &local_string_en_US;
}

#endif
