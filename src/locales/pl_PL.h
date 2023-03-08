#ifndef pl_PL_H
#define pl_PL_H

#include "locale.h"

char *locale_pl_PL[] = {
  "Nie można otworzyć `%s`\n",
  "Nie można otworzyć gniazda\n",

  "Nie można połączyć z serwerem\n",
  "Nie można wysłać wiadomości\n",

  "Nie można przypisać adresu\n",
  "Nie można słuchać na adresie\n",
  "Niepoprawny adres\n",

  "Nie można utworzyć ikony\n",

  "Wszyscy",
  "Zamknij",

  "połączono",
  "rozłączono",
  "nieznany adres lub już połączony",

  "Gotowy",
  "W trakcie",
  "Problem",

  "Nieznany język `%s`\n",
  "Błąd w pliku konfiguracji\n",
  "Nieznane pole `%s`\n"
};

char *local_string_pl_PL(int i) {
  return locale_pl_PL[i];
}

void set_locale_pl_PL() {
  local_string_func = &local_string_pl_PL;
}

#endif
