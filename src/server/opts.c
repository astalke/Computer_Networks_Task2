#include "server/opts.h"

/*
 * Otrzymuje wskaźnik na seed. Ustawia go domyślną wartością.
 * Przerywa program w przypadku błędu w funkcji time.
 */
static void init_default_seed(uint32_t * restrict pseed) {
  // Inicjujemy seed domyślną wartością.
  errno = 0;
  *pseed = time(NULL);
  if (errno != 0) {
    fatal_err("parameters_init_global");
  }
}

// Funkcja pobierająca wartość portu z str. Przerywa program przy błędzie.
static void handle_portnum(struct params * restrict result, 
    char const * restrict str) {
  result->port = strtousi(str);
  if (my_errno != 0) {
    fatal("Incorrect port number.");
  }
}

// Funkcja pobierająca wartość ziarna z str. Przerywa program przy błędzie.
static void handle_seed(struct params * restrict result, 
    char const * restrict str) {
  result->seed = strtou32(str);
  if (my_errno != 0) {
    fatal("Incorrect seed value.");
  }
}

// Funkcja pobierająca szybkość obrotu z str. Przerywa program przy błędzie.
static void handle_turning_speed(struct params * restrict result, 
    char const * restrict str) {
  result->turning_speed = strtou32(str);
  if (my_errno != 0 || result->turning_speed == 0 ||
      result->turning_speed > MAX_TURNING_SPEED) {
    fatal("Incorrect turning speed value.");
  }
}

// Funkcja pobierająca szybkość gry z str. Przerywa program przy błędzie.
static void handle_rounds_per_second(struct params * restrict result, 
    char const * restrict str) {
  result->rounds_per_sec = strtou32(str);
  if (my_errno != 0 || result->rounds_per_sec == 0 ||
      result->rounds_per_sec > MAX_GAME_SPEED) {
    fatal("Incorrect rounds per second value.");
  }
}

// Funkcja pobierająca szerokość planszy z str. Przerywa program przy błędzie.
static void handle_width(struct params * restrict result, 
    char const * restrict str) {
  result->width = strtou32(str);
  if (my_errno != 0 || result->width == 0 || 
      result->width > BOARD_MAX_WIDTH) {
    fatal("Incorrect width value.");
  }
}

// Funkcja pobierająca wysokość planszy z str. Przerywa program przy błędzie.
static void handle_height(struct params * restrict result, 
    char const * restrict str) {
  result->height = strtou32(str);
  if (my_errno != 0 || result->height == 0 || 
      result->height > BOARD_MAX_HEIGHT) {
    fatal("Incorrect height value.");
  }
}


struct params parameters_init_global(int argc, char *argv[]) {
  // Inicjujemy strukturę domyślnymi wartościami.
  struct params result = {.seed = 0, .turning_speed = 6, 
                              .port = 2021, .rounds_per_sec = 50, 
                              .width = 640, .height = 480};

  optind = 1;  
  init_default_seed(&result.seed);

  int c;
  while ((c = getopt (argc, argv, "+p:s:t:v:w:h:")) != -1) {
    my_errno = 0;
    switch (c) {
      // Numer portu.
      case 'p':
        handle_portnum(&result, optarg);        
        break;
      // Ziarno generowania liczb pseudolosowych.
      case 's':
        handle_seed(&result, optarg);
        break;
      // Szybkość skrętu.
      case 't':
        handle_turning_speed(&result, optarg);
        break;
      // Szybkość gry.
      case 'v':
        handle_rounds_per_second(&result, optarg);
        break;
      // Szerokość planszy.
      case 'w':
        handle_width(&result, optarg); 
        break;
      // Wysokość planszy.
      case 'h':
        handle_height(&result, optarg);
        break;
      // Nieznany argument.
      case '?':
        // getopt samo wypisze błąd.
        exit(EXIT_FAILURE);
        break;
      default:
        // Niemożliwe, przerwij program.
        assert(false);
        break;
    }
  }
  if (optind != argc) {
    fatal("Unknown argument.");
  }
  return result;
}
