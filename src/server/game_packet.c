#include "server/game.h"

// Wynik funkcji game_is_equal_player
enum giep_ret {GIEP_NOT_EQUAL, GIEP_RECONNECT, GIEP_OK, GIEP_IGNORE};

/*
 * Zwraca true, jeśli dane o gnieździe sobie odpowiadają.
 */
static bool game_is_equal_socket(struct sockaddr_in6 const *first, 
    struct sockaddr_in6 const *second) {
  assert(first != NULL);
  assert(second != NULL);
  if (first->sin6_port == second->sin6_port) {
    return memcmp(first->sin6_addr.s6_addr, second->sin6_addr.s6_addr, 16) == 0;
  }
  return false;
}
/*
 * Zwraca GIEP_NOT_EQUAL, jeśli dane pakietu nie odpowiadają graczowi.
 * Zwraca GIEP_RECONNECT, jeśli zgadzają się dane gniazda, ale nie session_id.
 * Zwraca GIEP_OK, jeśli dane pakietu w pełni odpowiadają graczowi.
 * Zwraca GIEP_IGNORE, jeśli dane pakietu powinny być zignorowane.
 */
static enum giep_ret game_is_equal_player(struct player const *p, 
    struct cl2serv const *data, struct sockaddr_in6 const *sock_data) {
  assert(p != NULL);
  assert(data != NULL);
  assert(sock_data != NULL);
  if (game_is_equal_socket(&p->sock_data, sock_data)) {
    if (p->session_id == data->session_id) {
      return GIEP_OK;
    } else {
      // Dotarł do nas stary pakiet.
      if (p->session_id < data->session_id) {
        return GIEP_IGNORE;
      }
      /*
       * Są różne od siebie, i p->session_id >= data->session_id
       * Oznacza to, że odłączamy starego gracza i podłączamy nowego.
       */
      return GIEP_RECONNECT;
    }
  }
  return GIEP_NOT_EQUAL;
}

/*
 * Wysyła dane do gracza o które prosił, począwszy od fst.
 */
static void game_send_in_bulk(struct game *g, struct player *p, uint32_t fst) {
  assert(g != NULL);
  assert(p != NULL);
  size_t n = vector_size(&g->events);
  // Jeśli jest co wysyłać - wysyłamy.
  if (fst < n) {
    // Bufor do którego wpisujemy dane pakietu.
    unsigned char buffer[MAX_PACKET_SIZE];
    // Aktualna ilość wolnego miejsca.
    size_t free_space = MAX_PACKET_SIZE;
    // Numer gry po przerobieniu na sieciową kolejność bajtów.
    uint32_t game_id = htobe32(g->game_id);
    // Czy pakiet jest nowy.
    bool new_packet = true;
    for (size_t i = fst; i < n; ++i) {
      // Jeśli pakiet nie jest nowy - spróbuj zapakować do niego dane.
      if (!new_packet) {
        bool res = event_serialise(vector_get(&g->events, i), 
            buffer + MAX_PACKET_SIZE - free_space, &free_space);
        // Teraz mogło się to nie powieść lub powieść.
        if (!res) {
          game_send_to_player(g, p, buffer, MAX_PACKET_SIZE - free_space); 
        }                
      }
      if (new_packet) {
        free_space = MAX_PACKET_SIZE;
        // Pakujemy numer gry.
        (void) memcpy(buffer, &game_id, sizeof(game_id));
        free_space -= sizeof(game_id);
        // Pakujemy pakiet - nie jest możliwe, by się to nie powiodło.
        bool res = event_serialise(vector_get(&g->events, i), 
          buffer + MAX_PACKET_SIZE - free_space, &free_space);
        assert(res);
        new_packet = false;
      }
    }    
    if (!new_packet) {
      game_send_to_player(g, p, buffer, MAX_PACKET_SIZE - free_space); 
    }
  }
}

/*
 * Obsługuje gracza, który jest podłączony.
 */
static void game_handle_existing(struct game *g, struct player *p,
    struct cl2serv const *data) {
  assert(g != NULL);
  assert(p != NULL);
  assert(data != NULL);
  // Czy minęły co najmniej 2 sekundy od ostatniej aktywności gracza?
  if (player_is_inactive(p)) {
    // Tak - rozłączamy gracza (i gubimy pakiet).
    game_disconnect_player(g, p);
    return;
  }
  // Zaznaczamy, że gracz wysłał do nas pakiet.
  player_touch(p);
  // Jeśli gra trwa, to ustawiamy kierunek ruchu gracza.
  if (g->flags & GAME_ONGOING) {
    // Nic nie szkodzi ustawić. To czy ma prawo sprawdzamy gdzie indziej.
    p->next_move = data->turn_direction;
    // Czy gracz odbiera komunikaty.
    if (p->flags & PLAYER_RECEIVER) {
      game_send_in_bulk(g, p, data->next_expected_event_no);      
    }
  } else {
    p->next_move = data->turn_direction;
    // Interesują nas dane tylko graczy oczekujących.
    if (p->queue == QUEUE_WAITING) {
      // Gracz wysłał strzałkę => Jest gotowy.
      if (data->turn_direction != DIRECTION_FORWARD) {
        p->flags |= PLAYER_READY;
      }
    }
    if (game_can_start(g)) {
      // Można zacząć grę
      game_start_game(g);      
    }
  }
}

// Zwraca true, jeśli nie ma gracza o tym samym id.
static bool game_is_player_unique(struct game *g, struct player *p) {
  for (size_t i = 0; i < g->in_game_counter; ++i) {
    // Sprawdzamy hasze
    if (p->hash == g->in_game[i].hash) {
      // Hasze identyczne - sprawdzamy znaki.
      if (strncmp(p->player_name, g->in_game[i].player_name, 
            PLAYER_NAME_MAX_LENGTH) == 0) {
        return false;
      }
    }
  }
  for (size_t i = 0; i < g->waiting_counter; ++i) {
    // Sprawdzamy hasze
    if (p->hash == g->waiting[i].hash) {
      // Hasze identyczne - sprawdzamy znaki.
      if (strncmp(p->player_name, g->waiting[i].player_name, 
            PLAYER_NAME_MAX_LENGTH) == 0) {
        return false;
      }
    }
  }
  return true;
}

/*
 * Podłącza gracza, jeśli to możliwe. Jeśli nie jest, to nic nie robi.
 */
static void game_connect_player(struct game *g, struct cl2serv const *data, 
    struct sockaddr_in6 const *sock_data) {
  assert(g != NULL);
  assert(data != NULL);
  assert(sock_data != NULL);
  if (g->num_of_players < PLAYER_MAX_NUMBER) {
    ++g->num_of_players;
    // Gracz zaczyna na kolejce oczekujących jeśli ma niepustą nazwę gracza.
    struct player p, *ptr;
    bool player_created = false;
    if (data->player_name_length > 0) {
      // Ta wartość nie powinna przekroczyć rozmiaru tablicy.
      assert(g->waiting_counter < PLAYER_MAX_NUMBER);
      p = player_create(data, sock_data, QUEUE_WAITING, g->waiting_counter);
      if (game_is_player_unique(g, &p)) {
        if (data->turn_direction == DIRECTION_LEFT || 
            data->turn_direction == DIRECTION_RIGHT) {
          p.next_move = data->turn_direction;
          p.flags |= PLAYER_READY;
        }
        g->waiting[g->waiting_counter] = p;
        ptr = &g->waiting[g->waiting_counter++]; 
        player_created = true;
      }
    } else {
      // Ta wartość nie powinna przekroczyć rozmiaru tablicy.
      assert(g->watching_counter < PLAYER_MAX_NUMBER);
      p = player_create(data, sock_data, QUEUE_WATCHING, g->watching_counter);
      // Obserwujących nie dotyczy wymaganie wyjątkowości.
      g->watching[g->watching_counter] = p;
      ptr = &g->watching[g->watching_counter++];
      player_created = true;
    }
    if (player_created) {
      // Zaczął istnieć - trzeba obsłużyć co przysłał.
      game_handle_existing(g, ptr, data);
    }
  }
  game_kick_inactive(g);
  if (!(g->flags & GAME_ONGOING)) {
    if (game_can_start(g)) {
      // Można zacząć grę
      game_start_game(g);      
    }
  }
}



/*
 * Obsługuje gracza, który jest podłączony, ale wymaga ponownego podłączenia.
 */
static void game_handle_reconnecting(struct game *g, struct player *p,
    struct cl2serv const *data, struct sockaddr_in6 const *sock_data) {
  assert(g != NULL);
  assert(p != NULL);
  assert(data != NULL);
  assert(sock_data != NULL);
  game_disconnect_player(g, p);
  game_connect_player(g, data, sock_data);
}

/*
 * Przetwarza gracza danego wektorem graczy i jego pozycją.
 * Zwraca true, jeśli wszystko jest załatwione i główna pętla może zakończyć
 * pracę. False jeśli to nie był poszukiwany gracz.
 */
static bool game_handle_player(struct game *g, struct player *p,
    struct cl2serv const *data, struct sockaddr_in6 const *sock_data) { 
  assert(g != NULL);
  assert(p != NULL);
  assert(data != NULL);
  assert(sock_data != NULL);
  // Ignorujemy rozłączoncyh graczy.
  if (p->flags & PLAYER_DISCONNECTED) {
    return false;
  }
  switch(game_is_equal_player(p, data, sock_data)) {
    // Gracz znaleziony! Można go obsłużyć.
    case GIEP_OK:
      game_handle_existing(g, p, data);
      return true;
      break;
    // Gracz znaleziony, rozłączyć i podłączyć nowego.
    case GIEP_RECONNECT:
      game_handle_reconnecting(g, p, data, sock_data);
      return true;
      break;
    // Gracz znaleziony, ale do zignorowania.
    case GIEP_IGNORE:
      return true;
      break;
    // Nie ten gracz. Szukamy dalej.
    case GIEP_NOT_EQUAL:
      return false;
      break;
    default:
      // Błąd programisty.
      assert(false);
      break;
  }
}

void game_handle_packet(struct game *g, struct cl2serv const *data, 
    struct sockaddr_in6 const *sock_data) {
  assert(g != NULL);
  assert(data != NULL);
  assert(sock_data != NULL);
  // Przechodzimy po wszystkich graczach.
  for (size_t i = 0; i < g->in_game_counter; ++i) {
    if (game_handle_player(g, &g->in_game[i], data, sock_data)) {
      // Gracz został znaleziony i obsłużony.
      return;
    }
  }
  for (size_t i = 0; i < g->waiting_counter; ++i) {
    if (game_handle_player(g, &g->waiting[i], data, sock_data)) {
      // Gracz został znaleziony i obsłużony.
      return;
    }
  }
  for (size_t i = 0; i < g->watching_counter; ++i) {
    if (game_handle_player(g, &g->watching[i], data, sock_data)) {
      // Gracz został znaleziony i obsłużony.
      return;
    }
  }
  // Nie znaleziono gracza - nowy gracz do podłączenia.
  game_connect_player(g, data, sock_data);
}


