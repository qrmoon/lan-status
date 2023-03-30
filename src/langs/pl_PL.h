#ifndef pl_PL_H
#define pl_PL_H

#include "lang.h"

char *lang_pl_PL[] = {
  "Nie można otworzyć `%s`: %s\n",
  "Nie można otworzyć gniazda: %s\n",

  "Nie można połączyć z serwerem: %s\n",
  "Nie można wysłać wiadomości: %s\n",

  "Nie można przypisać adresu: %s\n",
  "Nie można słuchać na adresie: %s\n",
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
  return lang_pl_PL[i];
}

void set_lang_pl_PL() {
  local_string_func = &local_string_pl_PL;
}

#endif
