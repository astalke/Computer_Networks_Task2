#ifndef OPTS_H
#define OPTS_H

#include "common/common.h"

// Maksymalne wymiary planszy.
#define BOARD_MAX_WIDTH         4000
#define BOARD_MAX_HEIGHT        4000
// Maksymalna szybkość skrętu
#define MAX_TURNING_SPEED       90
// Maksymalna szybkość gry.
#define MAX_GAME_SPEED          250

/*
 * Stuktura przechowująca informacje pozyskane z parametrów wywołania programu.
 */
struct params {
  uint32_t seed;          // Ziarno dla generatora liczb pseudolosowych.
  uint32_t turning_speed; // Szybkość skrętu robaka.
  uint32_t rounds_per_sec;// Szybkość gry.
  uint32_t width;         // Szerokość planszy w pikselach.
  uint32_t height;        // Wysokość planszy w pikselach.
  unsigned short int port;// Numer portu.
};

/*
 * Funkcja zwracająca strukturę params stworzoną z argumentów przekazanych
 * programowi i wartości domyślnych.
 */
struct params parameters_init_global(int argc, char *argv[]);

#endif /*OPTS_H*/
