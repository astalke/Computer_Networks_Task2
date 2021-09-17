#include "client/game.h"
#include "client/connect.h"
#include "client/gui2cl.h"

/*
 * Inicjuje zegar. 
 * Przerywa program w przypadku błędu.
 */
static int client_clock_create(void) {
  int result = timerfd_create(CLOCK_REALTIME, 0);
  if (result < 0) {
    fatal_err(__func__);    
  }
  return result;
}

static int game_clock_start(void) {
  int timr = client_clock_create();
  // Struktura zawierająca czas jaki mamy czekać.
  struct timespec t;
  t.tv_sec = 0;
  // W milisekundzie jest 10^6 nanosekund.
  t.tv_nsec = 30000000; // 30ms
  struct itimerspec new_value = {.it_interval = t, .it_value = t};
  if (timerfd_settime(timr, 0, &new_value, NULL) < 0) {
    fatal_err(__func__);    
  }
  return timr;
}

static uint64_t game_init_session_id(void) {
  uint64_t result = 0;
  struct timeval now;
  if (gettimeofday(&now, NULL) != 0) {
    fatal_err(__func__);
  }
  result += now.tv_sec;
  result *= 1000000;
  result += now.tv_usec;
  return result;
}

// Ustawia kierunek ruchu zależnie od przyciśniętych guzików.
static void game_set_direction(struct game *g) {
  bool const left = g->pressed[PRESSED_LEFT];
  bool const right = g->pressed[PRESSED_RIGHT];
  if (left && (!right)) {
    g->turn_direction = DIRECTION_LEFT;
  } else if ((!left) && right) {
    g->turn_direction = DIRECTION_RIGHT;
  } else {
    g->turn_direction = DIRECTION_FORWARD;
  }
}

// Obsłuży komunikat GUI.
static void game_handle_gui_message(struct game *g, enum gui_message mes) {
  switch (mes) {
    case GUI_LEFT_KEY_DOWN:
      g->pressed[PRESSED_LEFT] = true;
      g->pressed[PRESSED_RIGHT] = false;
      break;
    case GUI_LEFT_KEY_UP:
      g->pressed[PRESSED_LEFT] = false;
      break;
    case GUI_RIGHT_KEY_DOWN:
      g->pressed[PRESSED_RIGHT] = true;
      g->pressed[PRESSED_LEFT] = false;
      break;
    case GUI_RIGHT_KEY_UP:
      g->pressed[PRESSED_RIGHT] = false;
      break;
    default:
      assert(false);
      break;
  }
  game_set_direction(g);
}

// Wysyła pakiet od głównego serwera.
static void game_send_to_server(struct game *g, void const *buffer, 
    size_t length) {
  ssize_t res = send(g->server_sock, buffer, length, 0);
  if (res == -1 || (res > 0 && (size_t)res != length)) {
    if (res == -1) {
      fatal_err(__func__);
    } else {
      fprintf(stderr, "Partial send\n");
    }
  }
}

static void game_clear_players(struct game *g) {
  for (size_t i = 0; i < g->num_of_players; ++i) {
    free(g->players[i]);
    g->players[i] = NULL;
  }
  g->num_of_players = 0;
}

// Inicjuje nową grę i wysyła o tym informację do GUI.
static void game_send_new_game_to_gui(struct game *g, 
    struct event const *event) { 
  struct event_data_new_game_client *data = event->event_data;
  // Tyle na pewno starczy
  char message [512 + MAX_PACKET_SIZE];

  size_t offset = 0;
  offset += sprintf(message + offset, "NEW_GAME %"PRIu32" %"PRIu32, 
      data->maxx, data->maxy);
  g->maxx = data->maxx;
  g->maxy = data->maxy;
  
  game_clear_players(g);
  // Tylu graczy starczy.
  size_t num_of_players = 0;
  
  size_t j = 0;
  for (size_t i = 0; i < data->players_length; ++i) {
    if (data->players[i] == '\0') {
      // j, j+1, ..., i - 1 to nazwa gracza
      if (asprintf(&g->players[num_of_players], "%s", data->players + j) == 0) {
        fatal("Empty player name");
      }
      if (strcmp(g->players[num_of_players], g->player_name) == 0) {
        g->id = num_of_players;
      }
      offset += sprintf(message + offset, " %s", g->players[num_of_players++]);
      j = i + 1;
    }
  }
  offset += sprintf(message + offset, "\n");
  g->num_of_players = num_of_players;
  if (g->num_of_players < 2) {
    fatal("Not enought players.");
  }

  if (write_all(g->gui_sock, message, offset) < 0) {
    fatal("Error or lost connection to GUI\n");
  }
}

static void game_send_pixel_to_gui(struct game *g, 
    struct event const *event) {
  struct event_data_pixel *data = event->event_data;
  char message [512 + MAX_PACKET_SIZE];
  size_t offset = 0;
  uint32_t const x = data->x;
  uint32_t const y = data->y;
  uint8_t const pn = data->player_number;
  // Czy nie ma za dużo graczy?
  if (pn >= g->num_of_players) {
    fatal("Invalid data received from server\n");
  }
  // Czy gracz nie wyszedł poza pole?
  if (x >= g->maxx || y >= g->maxy) {
    fatal("Invalid data received from server\n");
  }
  
  offset += sprintf(message + offset, "PIXEL %"PRIu32" %"PRIu32" %s\n", x, y, 
      g->players[pn]);
  if (write_all(g->gui_sock, message, offset) < 0) {
    fatal("Error or lost connection to GUI\n");
  }
}

static void game_send_player_eliminated_to_gui(struct game *g, 
    struct event const *event) {
  struct event_data_player_eliminated *data = event->event_data;
  char message [512 + MAX_PACKET_SIZE];
  size_t offset = 0;
  // Czy nie ma za dużo graczy?
  if (data->player_number >= g->num_of_players) {
    fatal("Invalid data received from server\n");
  }
  offset += sprintf(message + offset, "PLAYER_ELIMINATED %s\n", 
      g->players[data->player_number]);

  if (write_all(g->gui_sock, message, offset) < 0) {
    fatal("Error or lost connection to GUI\n");
  }
}

static void game_send_to_gui(struct game *g, struct event const *event) {
  switch(event->event_type) {
    case NEW_GAME:
      game_send_new_game_to_gui(g, event);
      break;
    case PIXEL:
      game_send_pixel_to_gui(g, event);
      break;
    case PLAYER_ELIMINATED:
      game_send_player_eliminated_to_gui(g, event);
      break;
    case GAME_OVER:
      break;
    default:
      break;
  }
}

// Wykonuje stosowne działania zależne od wydarzeń.
static void game_handle_events(struct game *g, struct event *events, 
    size_t num) {
  for (size_t i = 0; i < num; ++i) {
    // Ignorujemy stare pakiety.
    // Jeśli jest zbyt nowy, to ignorujemy go i prosimy o retransmisję.
    if (events[i].event_no == g->next_expected_event_no) {
      // Nowy pakiet.
      game_send_to_gui(g, &events[i]);
      // Jeśli pakiet informował o zakończeniu gry - zakończ ją.
      if (events[i].event_type == GAME_OVER) {
        g->next_expected_event_no = 0;
        g->in_game = false;
      } else {
        ++g->next_expected_event_no;
      }
    }
    event_destroy(&events[i]);
  }
}

void game_destroy(struct game *g) {
  if (g != NULL) {
    game_clear_players(g);
    close(g->server_sock);
    close(g->gui_sock);
    close(g->timr);
  }
}

void game_handle_packet(struct game *g) {
  size_t const buf_size = MAX_PACKET_SIZE + 10;
  uint8_t buffer[buf_size];
  ssize_t len = recv(g->server_sock, buffer, buf_size, 0);
  if (len > 0) {
    struct event events[1 + (MAX_PACKET_SIZE - 4) / 13];
    uint32_t game_id = 0;
    size_t num = event_unserialise(events, buffer, len, &game_id);
    // Czy game_id dotyczy aktualnej gry?
    if (g->in_game && (game_id == g->game_id)) {
      game_handle_events(g, events, num);
    } 
    // Czy game_id to nie jest zgubiony pakiet?
    else if ((!g->in_game) && (game_id != g->game_id)) {
      // Nowa gra - nowy pakiet, nowej gry
      // Ustawienie next_expected_event_no na 0 wymusza retransmisję.
      g->next_expected_event_no = 0;
      g->old_game_id = g->game_id;
      g->game_id = game_id;
      g->in_game = true;
      game_handle_events(g, events, num);
    } else if ((g->in_game) && (game_id != g->game_id) && 
        (game_id != g->old_game_id)) {
      // Nowa gra - przegapiliśmy Game Over
      // Ustawienie next_expected_event_no na 0 wymusza retransmisję.
      g->next_expected_event_no = 0;
      g->old_game_id = g->game_id;
      g->game_id = game_id;
      g->in_game = true;
      game_handle_events(g, events, num);
    }
  } else {
    if (len < 0) {
      fatal_err(__func__);
    }
  }
}

void game_handle_gui(struct game *g) {
  enum gui_message mes = GUI_INVALID;
  while(tcp_read_message(g->gui_sock, &mes)) {
    game_handle_gui_message(g, mes);    
  }
}

struct game game_create(struct params const *params) {
  struct game result;
  result.maxx = 0;
  result.maxy = 0;
  result.id = 0;
  result.num_of_players = 0;
  result.in_game = false;
  result.game_id = 0;
  result.old_game_id = 0xFFFFFFFF;
  // Inicjowanie session_id
  result.session_id = game_init_session_id();
  result.server_sock = client_ipv6v4_udp_init(params->port, 
      params->server_name);
  result.gui_sock = client_ipv6v4_tcp_init(params->gui_port, params->gui_name);
  // Inicjowanie zegara.
  result.timr = game_clock_start();
  // Inicjowanie nazy gracza.
  memcpy(result.player_name, params->player_name, params->player_name_length);
  result.player_name_length = params->player_name_length;
  result.player_name[result.player_name_length] = 0;
  result.next_expected_event_no = 0;
  // Zaczynamy idąc do przodu.
  result.turn_direction = DIRECTION_FORWARD;

  // Przyciski GUI
  result.pressed[PRESSED_LEFT] = false;
  result.pressed[PRESSED_RIGHT] = false;

  return result;
}

void game_send_status(struct game *g) {
  // Maksymalny rozmiar pakietu + dodatkowe bajty
  size_t const max_packet_size = cl2serv_size() + 10;
  // Bufor na pakiet do wysłania.
  uint8_t buffer[max_packet_size];
  struct cl2serv result;
  result.session_id = g->session_id;
  result.next_expected_event_no = g->next_expected_event_no;
  result.turn_direction = g->turn_direction;
  result.player_name_length = g->player_name_length;
  strncpy(result.player_name, g->player_name, g->player_name_length);
  size_t length = cl2serv_serialise(&result, buffer);
  game_send_to_server(g, buffer, length);
}


