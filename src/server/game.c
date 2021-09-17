#include "server/game.h"

/*
 * Usuwa gracza p z kolejki (bez znaczenia na jakiej jest).
 * Zakłada, że kolejność nie ma znaczenia.
 */
static void game_remove_from_queue(struct game *g, struct player *p) {
  assert(g != NULL);
  assert(p != NULL);
  size_t *size;
  struct player *queue = game_get_queue(g, p->queue, &size);
  uint8_t id = p->id;
  player_destroy(p);
  --*size;
  --g->num_of_players;
  if (id != *size) {
    // Na miejsce skasowanego gracza wstawiamy ostatniego gracza.
    // Ostatni gracz znajduje się aktualnie na *size;
    // Wykonujemy płytkie kopiowanie.
    *p = queue[*size];
  }
}

/*
 * Buduje pojedynczy pakiet do przesyłania wydarzenia [event]
 * Umieszcza go w [buffer], a jego rozmiar w [size].
 */
static void game_build_single_packet(struct game *g, uint8_t *buffer, 
    size_t *size, struct event *event) {
  uint32_t game_id = htobe32(g->game_id);
  size_t free_space = MAX_PACKET_SIZE;
  // Pakujemy numer gry.
  (void) memcpy(buffer, &game_id, sizeof(game_id));
  free_space -= sizeof(game_id);
  bool result = event_serialise(event, buffer + sizeof(game_id), &free_space);
  // Musi się powieść.
  assert(result);
  *size = MAX_PACKET_SIZE - free_space;
}

// Rozsłya pakiet do wszystkich uprawnionych do odbierania komunikatów.
static void game_broadcast_event(struct game *g, struct event *event) {
  uint8_t buffer[MAX_PACKET_SIZE];
  size_t size;
  game_build_single_packet(g, buffer, &size, event);
  for (size_t i = 0; i < g->in_game_counter; ++i) {
    game_send_to_player(g, &g->in_game[i], buffer, size);
  }
  for (size_t i = 0; i < g->waiting_counter; ++i) {
    game_send_to_player(g, &g->waiting[i], buffer, size);
  }
  for (size_t i = 0; i < g->watching_counter; ++i) {
    game_send_to_player(g, &g->watching[i], buffer, size);
  }
}

static struct event game_event_init(struct game *g, enum event_type_t et,
    void *event_data) {
  size_t event_no = vector_size(&g->events);
  return event_create(event_no, et, event_data);
}

static void game_send_new_game(struct game *g) {
  char *players[PLAYER_MAX_NUMBER];
  // Inicjujemy tablicę players.
  for (size_t i = 0; i < g->in_game_counter; ++i) {
    struct player *p = &g->in_game[i];
    players[i] = p->player_name;
  }

  // Inicjujemy dane nowej gry.
  struct event_data_new_game *event_data = event_data_new_game_create(g->width,
      g->height, g->in_game_counter, players);

  struct event result = game_event_init(g, NEW_GAME, event_data);
  // Wstawiamy wydarzenie na kolejkę.
  vector_pushback(&g->events, &result);
  // I rozsyłamy wszystkim.
  game_broadcast_event(g, &result);
}

static void game_send_pixel(struct game *g, struct player *p, uint32_t x, 
    uint32_t y) {
  struct event_data_pixel *event_data = event_data_pixel_create(p->id, x, y);
  struct event result = game_event_init(g, PIXEL, event_data);
  // Wstawiamy wydarzenie na kolejkę.
  vector_pushback(&g->events, &result);
  // I rozsyłamy wszystkim.
  game_broadcast_event(g, &result);
}

static void game_send_player_eliminated(struct game *g, struct player *p) {
  struct event_data_player_eliminated *event_data = 
    event_data_player_eliminated_create(p->id);
  struct event result = game_event_init(g, PLAYER_ELIMINATED, event_data);
  // Wstawiamy wydarzenie na kolejkę.
  vector_pushback(&g->events, &result);
  // I rozsyłamy wszystkim.
  game_broadcast_event(g, &result);
}

static void game_kill_player(struct game *g, struct player *p) {
  if (p->flags & PLAYER_ALIVE) {
    // Nie chcemy zabić zwyciężcy.
    if (g->alive > 1) {
      p->flags ^= PLAYER_ALIVE;
      --g->alive;
      game_send_player_eliminated(g, p);
    }
  }
}

/*
 * Gracz [p] próbuje zjeść piksel na którym stoi. 
 * Jeśli mu się uda: wysyła PIXEL,
 * jeśli mu się nie uda: PLAYER_ELIMINATED.
 */
static void game_player_eat_pixel(struct game *g, struct player *p) {
  uint32_t x = (uint32_t)p->x;
  uint32_t y = (uint32_t)p->y;
  if (p->x >= 0.0 && p->y >= 0.0) {
    if (x < g->width) { 
      if (y < g->height) {
        if (!g->board[x][y]) {
          // PIXEL
          g->board[x][y] = 1;
          if (g->alive > 1) {
            game_send_pixel(g, p, x, y);
          }
          return;
        }
      }
    }
  }
  // ELIMINATED
  game_kill_player(g, p);
}

static void game_clear_board(struct game *g) {
  for (size_t i = 0; i < g->width; ++i) {
    memset(g->board[i], 0, g->height);
  }
}

/*
 * Sprawdza, czy gra powinna się zakończyć. Jeśli tak, to ją kończy.
 */
static void game_stop_if_game_over(struct game *g) {
  if (g->alive <= 1) {
    // GAME OVER
    if (g->flags & GAME_ONGOING) {
      g->flags ^= GAME_ONGOING;
    }
    struct event result = game_event_init(g, GAME_OVER, NULL);
    // Wstawiamy wydarzenie na kolejkę.
    vector_pushback(&g->events, &result);
    // I rozsyłamy wszystkim.
    game_broadcast_event(g, &result);
    // Przenosimy wszystkich graczy na kolejkę oczekujących.
    // Potem wyrzucimy nieaktywnych
    for (uint8_t i = 0; i < g->in_game_counter; ++i) {
      struct player *p = &g->waiting[g->waiting_counter];
      *p = g->in_game[i];
      if (p->flags & PLAYER_ALIVE) {
        p->flags ^= PLAYER_ALIVE;
      }
      if (p->flags & PLAYER_READY) {
        p->flags ^= PLAYER_READY;
      }
      // Zaznaczamy, że gracz jest na kolejce oczekujących.
      p->queue = QUEUE_WAITING;
      // Wpisujemy graczowi jego id.
      p->id = g->waiting_counter++;
      // Zaczyna bez wprowadzonego kierunku.
      p->next_move = DIRECTION_FORWARD;
    } 
    g->in_game_counter = 0;
    for (uint8_t i = 0; i < g->waiting_counter; ++i) {
      struct player *p = &g->waiting[i];
      if (p->flags & PLAYER_DISCONNECTED) {
        game_disconnect_player(g, p);
      }
    }
    // Czyścimy kolejkę komunikatów.
    vector_clear(&g->events, event_destroy);
    game_clear_board(g);
  }
}

static void game_handle_inactive_game(struct game *g) {
  assert(g != NULL);
  assert((g->flags & GAME_ONGOING) == 0);
  // Przechodzimy po wszystkich graczach - wyrzucamy niekatywnych.
  // Kolejka grających powinna być pusta.
  assert(g->in_game_counter == 0);
  game_kick_inactive(g);  
  /*
   * Sprawdzamy, czy wszyscy oczekujący gracze zgłosili chęć udziału.
   * I jest ich co najmniej 2.
   */
  if (game_can_start(g)) {
    game_start_game(g);
  }
}

/*
 * Funkcja pomocnicza do game_handle_active_game.
 * Obraca gracza zależnie od ostatniego komunikatu.
 * Ustawia kierunek na DIRECTION_FORWARD.
 */
static void game_rotate_player(struct game *g, struct player *p) {
  switch(p->next_move) {
    case DIRECTION_LEFT:
      p->direction -= g->turning_speed;
      p->direction = (p->direction + 360) % 360;
      break;
    case DIRECTION_RIGHT:
      p->direction += g->turning_speed;
      p->direction = (p->direction + 360) % 360;
      break;
    default:
      break;
    }
  //p->next_move = DIRECTION_FORWARD;
}

static void game_handle_active_game(struct game *g) {
  assert(g != NULL);
  assert((g->flags & GAME_ONGOING) == 1);
  game_kick_inactive(g);
  for (size_t i = 0; i < g->in_game_counter; ++i) {
    struct player *p = &g->in_game[i];
    if (p->flags & PLAYER_ALIVE) {
      // Obróć gracza.
      game_rotate_player(g, p);    
      // Przesuń gracza o 1 do przodu.
      if (player_move(p)) {
        // Wyślij PIXEL lub PLAYER_ELIMINATED
        game_player_eat_pixel(g, p); 
      }
    }
  }
  game_stop_if_game_over(g);
}

void game_start_game(struct game *g) {
  assert(g != NULL);
  server_clock_start(g->timr, g->rps);
  g->game_id = server_random_rand();
  g->flags |= GAME_ONGOING;
  // Zakładamy, że wszyscy oczekujący są aktywni i zgłosili chęć gry.
  // Sortujemy kolejkę oczekujących.
  qsort(g->waiting, g->waiting_counter, sizeof(struct player), 
      player_compare_for_qsort);
  // Posortowanych przenosimy do kolejki grających.
  for (uint8_t i = 0; i < g->waiting_counter; ++i) {
    struct player *p = &g->in_game[i];
    *p = g->waiting[i];
    ++g->in_game_counter;
    // Zaznaczamy, że gracz żyje i odbiera komuniakty.
    p->flags = PLAYER_RECEIVER | PLAYER_ALIVE;
    // Zaznaczamy, że gracz jest na kolejce żyjących.
    p->queue = QUEUE_IN_GAME;
    // Wpisujemy graczowi jego id.
    p->id = i;
  }
  g->waiting_counter = 0;
  g->alive = g->in_game_counter;
  game_send_new_game(g);
  for (uint8_t i = 0; i < g->in_game_counter; ++i) {
    struct player *p = &g->in_game[i];
    // Zaczynamy generowanie pozycji
    p->x = ((double)(server_random_rand() % g->width)) + 0.5;
    p->y = ((double)(server_random_rand() % g->height)) + 0.5;
    p->direction = server_random_rand() % 360;
    // Zje lub zginie.
    game_player_eat_pixel(g, p);
  }
  game_stop_if_game_over(g);
}



void game_handle_game(struct game *g) {
  assert(g != NULL);
  // Czy gra trwa?
  if (g->flags & GAME_ONGOING) {
    game_handle_active_game(g);
  } else {
    game_handle_inactive_game(g);
  }
}

// Wysyła pakiet od gracza.
void game_send_to_player(struct game *g, struct player *p, void const *buffer, 
    ssize_t asize) {
  size_t size = 0;
  size += asize;
  if (p->flags & PLAYER_RECEIVER) {
    // Wysyłamy pakiet.
    errno = 0;
    ssize_t len = sendto(g->sock, buffer, size, 0,
        (struct sockaddr *)&p->sock_data, sizeof(p->sock_data));
    // Wystąpił problem, nic nie możemy z tym zrobić.
    if (len != asize) {
      if (len == -1) {
        perror(__func__);
      } else {
        fprintf(stderr, "Partial sendto.\n");
      }
    }
  }
}

/*
 * Zwraca odpowiednią kolejkę. Do *[size] wrzuca wskaźnik na rozmiar tej kolejki
 */
struct player *game_get_queue(struct game *g, enum player_queue queue,
    size_t **size) {
  assert(g != NULL);
  assert(size != NULL);
  switch(queue) {
    case QUEUE_IN_GAME:
      *size = &g->in_game_counter;
      return g->in_game;
      break;
    case QUEUE_WAITING:
      *size = &g->waiting_counter;
      return g->waiting;
      break;
    case QUEUE_WATCHING:
      *size = &g->watching_counter;
      return g->watching;
      break;
    default:
      assert(false);
      break;
  }
}

/*
 * Odłącza gracza p, bierze pod uwagę gdzie jest.
 */
void game_disconnect_player(struct game *g, struct player *p) {
  assert(g != NULL);
  assert(p != NULL);
  switch (p->queue) {
    case QUEUE_IN_GAME:
      // Ta kolejka powinna istnieć, wtedy i tylko wtedy gdy gra trwa.
      assert((g->flags & GAME_ONGOING) == 0);
      // Nie można go po prostu wyrzucić - ustawiamy flagę, że jest odłączony.
      p->flags |= PLAYER_DISCONNECTED; 
      if (p->flags & PLAYER_RECEIVER) {
        p->flags ^= PLAYER_RECEIVER;
      }
      // Zmniejszamy licznik obsługiwanych graczy.
      --g->num_of_players;
      break;
    // Odłączanie pozostałych działa tak samo.
    case QUEUE_WAITING:
    case QUEUE_WATCHING:
      // Można gracza spokojnie wyrzucić.
      game_remove_from_queue(g, p);
      break;
    default:
      // Błąd programisty
      assert(false);
      break;
  }
}

struct game game_create(uint32_t width, uint32_t height, 
    uint32_t turning_speed, uint32_t rps, int sock, int timr) {
  struct game result;
  // Czyścimy wszystkie pola.
  memset(&result, 0, sizeof(result));
  // Tworzymy wektor wydarzeń.
  errno = 0;
  result.events = vector_create(sizeof(struct event));
  if (errno) {
    fatal_err(__func__);
  }

  // Inicjujemy planszę zerami.
  result.board = calloc(width, sizeof(*result.board));
  if (errno) {
    fatal_err(__func__);
  }
  result.width = width;
  result.height = height;
  result.turning_speed = turning_speed;
  result.sock = sock;
  result.timr = timr;
  result.rps = rps;

  size_t board_size = width;
  board_size *= height;
  result.board[0] = calloc(board_size, sizeof(char));
  if (errno) {
    fatal_err(__func__);
  }
  for (uint32_t i = 1; i < width; ++i) {
    result.board[i] = result.board[i - 1] + (sizeof(char) * height);
  }

  return result;
}

void game_destroy(struct game *g) {
  if (g != NULL) {
    vector_destroy(&g->events, event_destroy);
    free(g->board[0]);
    free(g->board);
  }
}

void game_kick_inactive(struct game *g) {
  for (size_t i = 0; i < g->in_game_counter; ++i) {
    struct player *p = &g->in_game[i];
    if (player_is_inactive(p)) {
      game_disconnect_player(g, p);
    }
  }
  for (size_t i = 0; i < g->waiting_counter; ++i) {
    struct player *p = &g->waiting[i];
    if (player_is_inactive(p)) {
      game_disconnect_player(g, p);
    }
  }
  for (size_t i = 0; i < g->watching_counter; ++i) {
    struct player *p = &g->watching[i];
    if (player_is_inactive(p)) {
      game_disconnect_player(g, p);
    }
  }
}

bool game_can_start(struct game *g) {
  if (g->waiting_counter < 2) {
    return false;
  }
  for (size_t i = 0; i < g->waiting_counter; ++i) {
    struct player *p = &g->waiting[i];
    if (!(p->flags & PLAYER_READY)) {
      return false;
    }
  }
  return true;
}


