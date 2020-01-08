#include <stdint.h>

#ifndef _HELPER_H
#define _HELPER_H

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

const uint8_t ADDRESS_SIZE = 20;

const int ERR_INVALID_ADDRESS = 127;

const int TRUE = 0;
const int FALSE = 1;

int is_valid_addr(uint8_t *addr) {
  if (ADDRESS_SIZE != ARRAY_LEN(&addr)) {
    return FALSE;
  }

  return TRUE;
}

int match_addr(uint8_t *addr1, uint8_t *addr2) {
  if (ADDRESS_SIZE != ARRAY_LEN(&addr1)) {
    return ERR_INVALID_ADDRESS;
  }

  if (ADDRESS_SIZE != ARRAY_LEN(&addr2)) {
    return ERR_INVALID_ADDRESS;
  }

  for (int i = 0; i < ADDRESS_SIZE; ++i) {
    if (addr1[i] != addr2[i]) {
      return FALSE;
    }
  }

  return TRUE;
}

#endif
