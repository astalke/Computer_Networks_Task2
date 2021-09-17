#ifndef PLAYER_H
#define PLAYER_H

#include "common/common.h"
#include "common/cl2serv.h"
#include "server/server_time.h"

/*
 * Rodzaje graczy:
 *  Żywy      - może poruszać się robakiem, otrzymuje komunikaty od serwera, 
 *              jego pozycja na kolejce ma znaczenie, odłączenie skutkuje 
 *              przejściem w stan "Odłączony". Po zakończeniu gry są przensozeni
 *              na kolejkę oczekujących.
 *  Martwy    - nie może poruszać się robakiem, otrzymuje komunikaty od serwera,
 *              jego pozycja na kolejce ma znaczenie, odłączenie skutkuje 
 *              przejściem w stan "Odłączony". Po zakończeniu gry zmieniają 
 *              stan na "Żywy".
 *  Czekający - nie może poruszać się robakiem, otrzymuje komunikaty od serwera,
 *              jego pozycja na kolejce nie ma znaczenia, odłączenie skutkuje
 *              usunięciem go z kolejki. Po rozpoczęciu gry zmieniają stan na
 *              "Żywy". Wszyscy muszą wyrazić zgodę na rozpoczęcie nowej gry.
 *  Obserwujący-nie może poruszać się robaiem, otrzymuje komunikaty od serwera,
 *              jego pozycja na kolejce nie ma znaczenia, odłączenie skutkuje
 *              usunięciem go z kolejki. Nie zmieniają stanu.
 *  Odłączony - nie może poruszać się robakiem, nie otrzymuje komunikatów.
 *              Jego pozycja na kolejce ma znaczenie. Jeśli nie trwa gra, to
 *              są usuwani z kolejki.
 */

// Flagi opisujące gracza

/*
 * Czy gracz otrzymuje powiadomienia od serwera.
 * True  dla: graczy żywych i martwych, obserwujących i czekających na grę.
 * False dla: graczy, którzy odłączyli się w trakcie gry.
 * Jeśli flaga jest false, to gracz jest usuwany z kolejki po zakończeniu gry.
 */
#define PLAYER_RECEIVER   0x01
/*
 * Czy gracz może sterować robakiem.
 * True   dla: graczy żywych.
 * False  dla: graczy martwych, obserwujących, czekających i odłączonych.
 */
#define PLAYER_ALIVE      0x02

/*
 * Czy gracz jest gotowy. Flaga ma sens tylko dla graczy oczekujących.
 * Flaga jest zerowana po rozpoczęciu nowej gry. Serwer pozwala na jej 
 * ustawienie tylko jak nie odbywa się żadna gra. Ustawiana jest, gdy gracz
 * oczekujący wyśle pakiet z turn_direction różnym od 0.
 */
#define PLAYER_READY      0x04

/*
 * True, jeśli gracz nie jest już podłączony.
 */
#define PLAYER_DISCONNECTED 0x08

// Kolejka, na której znajduje się gracz.
enum player_queue {QUEUE_IN_GAME, QUEUE_WAITING, QUEUE_WATCHING};

/*
 * Struktura gracza.
 */
struct player {
  // Pozycja robaka.
  double x, y;
  // Numer sesji gracza.
  uint64_t session_id;
  // Hasz nazwy gracza.
  uint64_t hash;
  size_t player_name_length;
  // Dane o gnieździe.
  struct sockaddr_in6 sock_data;
  // Dane o czasie ostatniej aktywności.
  struct timeval last_active;
  // Kierunek ruchu robaka.
  int16_t direction;
  uint16_t flags;
  // Kolejka na jakiej znajduje się gracz.
  enum player_queue queue;
  // Pozycja gracza na kolejce.
  uint8_t id;
  // Następny ruch gracza.
  enum turn_direction_t next_move;
  char player_name[PLAYER_NAME_MAX_LENGTH + 1];
};

/*
 * Ustawia graczowi [player] aktualny czas jako czas ostatniej aktywności.
 * Ustawia errno w przypadku problemów.
 */
void player_touch(struct player *player);

/*
 * Zwraca true, jeśli od ostatniej aktywności gracza minęły ponad 2 sekundy.
 */
bool player_is_inactive(struct player *player);

/*
 * Tworzy nowego gracza. 
 * W przypadku problemów, inicjowanie gracza nie zostanie dokończone i
 * ustawione zostanie errno.
 */
struct player player_create(struct cl2serv const *data, 
                            struct sockaddr_in6 const * restrict sock_data,
                            enum player_queue queue, uint8_t id);

// Destruktor gracza. Nic nie robi, bo gracz nie ma nic co tego wymaga.
void player_destroy(struct player *p);

// Komparator dla gracza, kompatybilny z qsort.
int player_compare_for_qsort(void const *first, void const *second);

// Funkcja przesuwająca gracza o 1 w aktualnym kierunku.
// Zwraca true, jeśli zmienił pozycję.
bool player_move(struct player *p);

#endif /*PLAYER_H*/
