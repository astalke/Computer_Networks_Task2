#include "common/common.h"

int my_errno = 0;

// Tablica trzymająca wskaźniki.
void ** ptrs_to_free;
// Długość powyższej tablicy.
size_t ptrs_to_free_len;
// Maksymalna długość powyższej tablicy.
size_t ptrs_to_free_max_len;

static void exit_cleanup(void) {
  for (size_t i = 0; i < ptrs_to_free_len; ++i) {
    free(ptrs_to_free[i]);
  }
  free(ptrs_to_free);
}

// Zwraca true, jeśli tekst jest liczbą.
static bool is_number(char const * restrict str) {
  for (size_t i = 0; str[i]; ++i) {
    if (!(str[i] <= '9' && str[i] >= '0')) {
      return false;      
    }
  }
  return true;
}

void exit_init(void) {
  ptrs_to_free_max_len = 8;
  ptrs_to_free = calloc(ptrs_to_free_max_len, sizeof(*ptrs_to_free));
  if (ptrs_to_free == NULL) {
    fatal_err("exit_init");
  }
  ptrs_to_free_len = 0;
  if (atexit(exit_cleanup) != 0) {
    fatal("Failed to set exit function.");
  }
}

bool exit_register_free(void *ptr) {
  if (ptrs_to_free_len >= ptrs_to_free_max_len) {
    void **temp = realloc(ptrs_to_free, 2 * ptrs_to_free_max_len);
    if (temp == NULL) {
      return false;
    }
    ptrs_to_free = temp;
    ptrs_to_free_max_len *= 2;
  }
  ptrs_to_free[ptrs_to_free_len++] = ptr;
  return true;
}

void fatal_err(char const *func) {
  perror(func);
  exit(EXIT_FAILURE);
}

void fatal(char const *mess) {
  fprintf(stderr, "ERROR: %s\n", mess);
  exit(EXIT_FAILURE);
}


unsigned short int strtousi(char const * restrict str) {
  unsigned short int result = 0;
  errno = 0;
  if (!is_number(str)) {
    errno = EINVAL;
    my_errno = EINVAL;
    return 0;
  } 
  unsigned long temp = strtoul(str, NULL, 10);
  // Wystąpił błąd przy parsowaniu lub wartość jest za duża.
  if (errno != 0 || temp > USHRT_MAX) {
    my_errno = EINVAL;
    return 0;
  }
  // Nie dojdzie do przepełnienia.
  result += temp;
  return result;
}

uint32_t strtou32(char const * restrict str) {
  uint32_t result = 0;
  errno = 0;
  if (!is_number(str)) {
    errno = EINVAL;
    my_errno = EINVAL;
    return 0;
  }
  unsigned long long temp = strtoull(str, NULL, 10);
  // Wystąpił błąd przy parsowaniu lub wartość jest za duża.
  if (errno != 0 || temp > UINT32_MAX) {
    my_errno = EINVAL;
    return 0;
  }
  // Nie dojdzie do przepełnienia.
  result += temp;
  return result;
}

bool validate_player_name(char const * restrict str) {
  size_t i = 0;
  for (; str[i]; ++i) {
    if (str[i] < 33 || str[i] > 126) {
      my_errno = EINVAL;
      return false;
    }
  }
  if (i > PLAYER_NAME_MAX_LENGTH) {
    my_errno = EINVAL;
    return false;
  }
  return i > 0;
}
