#ifndef OPTS_H
#define OPTS_H

#include "common/common.h"

/*
 * Stuktura przechowująca informacje pozyskane z parametrów wywołania programu.
 */
struct params {
  // Nazwa lub IP serwera, alokowane mallociem.
  char const *server_name;    
  // Nazwa gracza, . NULL jeśli obserwator
  char player_name[PLAYER_NAME_MAX_LENGTH + 1];    
  size_t player_name_length;
  // Nazwa lub IP serwera obsługującego GUI, alokowane mallociem.
  char const *gui_name;       
  // Numer portu.
  unsigned short int port;    
  // Numer portu serwera obsługującego GUI.
  unsigned short int gui_port;
};

/*
 * Funkcja zwracająca strukturę params stworzoną z argumentów przekazanych
 * programowi i wartości domyślnych.
 */
struct params parameters_init_global(int argc, char *argv[]);

#endif /*OPTS_H*/
