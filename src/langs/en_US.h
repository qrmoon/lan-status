#ifndef en_US_H
#define en_US_H

char *lang_en_US[] = {
  "Cannot open `%s`: %s\n",
  "Cannot open socket: %s\n",

  "Cannot connect to server: %s\n",
  "Cannot send message: %s\n",

  "Cannot bind address: %s\n",
  "Cannot listen on address: %s\n",
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

  "Unknown lang `%s`\n",
  "Error in config file\n",
  "Unknown field `%s`\n"
};

char *local_string_en_US(int i) {
  return lang_en_US[i];
}

void set_lang_en_US() {
  local_string_func = &local_string_en_US;
}

#endif
