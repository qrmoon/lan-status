#ifndef LOCALES_ALL_H
#define LOCALES_ALL_H

#include "en_US.h"
#include "pl_PL.h"

bool set_locale_str(char *l) {
  if (strcmp(l, "en_US") == 0)
    set_locale_en_US();
  else if (strcmp(l, "pl_PL") == 0)
    set_locale_pl_PL();
  else {
    set_locale_en_US();
    return false;
  }

  return true;
}

#endif
