#include "client/opts.h"

// Funkcja pobierająca wartość portu z str. Przerywa program przy błędzie.
static void handle_portnum(struct params * restrict result, 
    char const * restrict str) {
  result->port = strtousi(str);
  if (my_errno != 0) {
    fatal("Incorrect port number.");
  }
}

// Funkcja pobierająca wartość portu GUI z str. Przerywa program przy błędzie.
static void handle_gui_portnum(struct params * restrict result, 
    char const * restrict str) {
  result->gui_port = strtousi(str);
  if (my_errno != 0) {
    fatal("Incorrect port number.");
  }
}

// Funkcja alokuje miejsce, kopiuje do niego zawartość str i umieszcza w dest.
static void handle_malloc(char const **dest, char const * restrict str) {
  size_t n = 1 + strlen(str); // Argumenty są zakończone NULLem.
  char *result = malloc(n * sizeof(char));
  if (result == NULL) {
    fatal_err("handle_malloc");
  }
  // Rejestrujemy wynik malloc do free przed zakończeniem programu.
  if (!exit_register_free(result)) {
    fatal_err("handle_malloc");
  }
  strcpy(result, str);
  *dest = result;
}

// Funkcja pobierająca adres serwera z str. Przerywa program przy błędzie.
// Przesuwa wartość optind o 1 do przodu.
static void handle_server_name(struct params * restrict result, 
    char const * restrict str) {
  handle_malloc(&result->server_name, str);
  ++optind;
}

// Funkcja pobierająca adres serwera GUI z str. Przerywa program przy błędzie.
static void handle_gui_server_name(struct params * restrict result, 
    char const * restrict str) {
  handle_malloc(&result->gui_name, str);
}

// Funkcja pobierająca nazwę gracza z str. Przerywa program przy błędzie.
static void handle_player_name(struct params * restrict result, 
    char const * restrict str) {
  my_errno = 0;
  if (validate_player_name(str)) {
    result->player_name_length = strlen(str);
    memcpy(result->player_name, str, result->player_name_length);
    result->player_name[result->player_name_length] = 0;
  }
  if (my_errno == EINVAL) {
    fatal("Player name is invalid: too long or illegal characters.");
  }
}

struct params parameters_init_global(int argc, char *argv[]) {
  // Inicjujemy strukturę domyślnymi wartościami.
  struct params result = {.port = 2021, .gui_port = 20210, .server_name = NULL, 
                          .gui_name = "localhost"}; 
  optind = 1;

  // Pobieramy nazwę lub adres serwera.
  if (argc > 1) {
    handle_server_name(&result, argv[1]);
  } else {
    fatal("Not enough arguments.");
  }

  int c;
  bool player_name_set = false;
  while ((c = getopt (argc, argv, "+n:p:i:r:")) != -1) {
    my_errno = 0;
    switch (c) {
      // Nazwa gracza.
      case 'n':
        player_name_set = true;
        handle_player_name(&result, optarg);
        break;
      // Numer portu.
      case 'p':
        handle_portnum(&result, optarg);        
        break;
      // Nazwa serwera GUI.
      case 'i':
        handle_gui_server_name(&result, optarg);
        break;
      // Numer portu serwera GUI.
      case 'r':
        handle_gui_portnum(&result, optarg);
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
  if (!player_name_set) {
    fatal("Player name not set.");
  }
  return result;
}
