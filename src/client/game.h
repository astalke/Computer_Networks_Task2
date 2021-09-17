#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "common/common.h"
#include "client/opts.h"
#include "common/cl2serv.h"
#include "common/serv2cl.h"

#define PRESSED_LEFT  0
#define PRESSED_RIGHT 1

struct game {
  bool in_game;
  uint32_t maxx, maxy;
  uint32_t game_id, old_game_id;
  // Numer sesji gracza.
  uint64_t session_id;
  // Gniazda odpowiednio serwera i gui
  int server_sock, gui_sock;
  // Czasomierz
  int timr;
  // Nazwa gracza.
  char player_name[PLAYER_NAME_MAX_LENGTH + 1];
  // Długość nazwy gracza.
  size_t player_name_length;
  uint32_t next_expected_event_no;
  // Kierunek w jakim ustalono poruszanie się.
  enum turn_direction_t turn_direction;
  // pressed[PRESSED_LEFT] == True, gdy jest kliknięty lewy przycisk
  // Analogicznie pressed[PRESSED_RIGHT] gdy prawy
  bool pressed[2];
  char *players[MAX_PACKET_SIZE];
  size_t num_of_players;
  uint8_t id;
};

// Tworzy nowy stan gry.
struct game game_create(struct params const *params);

// Destruktor gry.
void game_destroy(struct game *g);

// Wysyła do serwera aktualny stan gracza.
void game_send_status(struct game *g);

// Przetwarza otrzymany od serwera pakiet.
void game_handle_packet(struct game *g);

// Przetwarza otrzymany od GUI pakiet.
void game_handle_gui(struct game *g);

#endif /*CLIENT_GAME_H*/
