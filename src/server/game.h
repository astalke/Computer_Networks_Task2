#ifndef GAME_H
#define GAME_H

#include "server/player.h"
#include "common/vector.h"
#include "common/serv2cl.h"
#include "server/random.h"

// Flaga oznaczająca, że gra trwa.
#define GAME_ONGOING  0x01
/*
 * Struktura stanu gry.
 */
struct game {
  // Tablica graczy, którzy są w grze. Mogą być żywi, martwi lub odłączeni. 
  struct player in_game[PLAYER_MAX_NUMBER];
  // Licznik graczy, którzy są w grze.
  size_t in_game_counter;
  // Tablica graczy oczekujących na grę.
  struct player waiting[PLAYER_MAX_NUMBER];
  // Licznik graczy, którzy czekają na grę.
  size_t waiting_counter;
  // Tablica graczy oglądającyh grę.
  struct player watching[PLAYER_MAX_NUMBER];
  // Licznik graczy, którzy oglądają grę.
  size_t watching_counter;
  /*
   * Łączna liczba graczy.
   * Liczba ta nie może przekroczyć PLAYER_MAX_NUMBER, ale może być różna od
   * sum rozmiarów kolejej gdy np. 23 graczy się odłączy w trakcie gry i 23 
   * zacznie czekać na grę.
   */
  size_t num_of_players;
  // Liczba żywych graczy.
  size_t alive;
  // Id gry.
  uint32_t game_id;
  // Flagi
  uint32_t flags;
  uint32_t width, height, turning_speed, rps;
  // Gniazdo i timer
  int sock, timr;
  // Wektor wydarzeń.
  struct vector events; 
  // Plansza board[x][y] - 0 oznacza, że pole jest niezjedzone.
  char **board;
};

/*
 * Funkcja tworząca strukturę gry. Powinna być wywołana RAZ, do zakończenia
 * gry lub rozpoczęcia nowej służą inne funkcje.
 * Przerywa program w przypadku błędu.
 */
struct game game_create(uint32_t width, uint32_t height, 
    uint32_t turning_speed, uint32_t rps, int sock, int timr);

/*
 * Destruktor gry.
 */
void game_destroy(struct game *g);

/*
 * Funkcja przetwarzająca pakiet otrzymany od gracza.
 */
void game_handle_packet(struct game *g, struct cl2serv const *data, 
    struct sockaddr_in6 const *sock_data);

/*
 * Funkcja obsługująca grę.
 */
void game_handle_game(struct game *g);

// Wysyła pakiet od gracza.
void game_send_to_player(struct game *g, struct player *p, void const *buffer, 
    ssize_t size);

/*
 * Zwraca odpowiednią kolejkę. Do *[size] wrzuca wskaźnik na rozmiar tej kolejki
 */
struct player *game_get_queue(struct game *g, enum player_queue queue,
    size_t **size);

void game_disconnect_player(struct game *g, struct player *p);

// Rozpoczyna grę.
void game_start_game(struct game *g);

// Wyrzuca nieaktywnych.
void game_kick_inactive(struct game *g);

// Zwraca true, jeśli wszyscy gracze są gotowi.
bool game_can_start(struct game *g);


#endif /*GAME_H*/
