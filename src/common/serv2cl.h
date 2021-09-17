#ifndef SERVER_TO_CLIENT_H
#define SERVER_TO_CLIENT_H
#include "common/common.h"
#include "common/crc.h"

// Maksymalny rozmiar pakietu serwera do klienta.
#define MAX_PACKET_SIZE 550

enum event_type_t {NEW_GAME, PIXEL, PLAYER_ELIMINATED, GAME_OVER, EV_INVALID};

// Struktura wydarzenia.
struct event {
  void *event_data;             // Wskaźnik na dane opisujące wydarzenie.
  uint32_t event_no;            // Numer wydarzenia.
  enum event_type_t event_type; // Jakie wydarzenie. 
};

// Wydarzenie: rozpoczęcie nowej gry.
struct event_data_new_game {
  uint32_t maxx;                // Szerokość planszy w pikselach.
  uint32_t maxy;                // Wysokość planszy w pikselach.
  // Liczba graczy
  uint8_t player_num;
  // Tablica nazw graczy.
  char players[PLAYER_MAX_NUMBER][PLAYER_NAME_MAX_LENGTH + 1/*\0*/];
};

// Wersja powyższego wydarzenia dla klienta.
struct event_data_new_game_client {
  uint32_t maxx;                // Szerokość planszy w pikselach.
  uint32_t maxy;                // Wysokość planszy w pikselach.
  // Dowolna liczba graczy, trzymamy po prostu to co przysłał serwer.
  // Dłuższe niż 550 bajtów to nie będzie, bo nie zmieści się w pakiecie.
  char players[MAX_PACKET_SIZE + 10];
  // Długość powyższej tablicy.
  size_t players_length;
};

// Wydarzenie: pixel
struct event_data_pixel {
  uint8_t player_number;        // Numer gracza.
  uint32_t x;                   // Odcięta
  uint32_t y;                   // Rzędna.
};

// Wydarzenie: PLAYER_ELIMINATED
struct event_data_player_eliminated {
  uint8_t player_number;        // Numer gracza. 
};

// Destruktor wydarzenia.
void event_destroy(void *event);

// Konstruktor wydarzenia.
struct event event_create(uint32_t event_no, enum event_type_t event_type,
    void *event_data);

/*
 * Funkcja przyjmuje wydarzenie [arg] i ładuje je do pamięci pod adres
 * [buf] w którym jest [*buf_size] wolnego miejsca.
 * Zwraca true jeśli powiodło się pakowanie. False, jeśli brakuje miejsca.
 * Jeśli zwróci true, to zmniejszy buf_size o odpowiednią wartość.
 * Ustawia my_errno jeśli pakiet jest niepoprawny.
 */
bool event_serialise(struct event const *arg, void * const restrict buf, 
    size_t *buf_size);

/*
 * Funkcja otrzymuje tablicę wydarzeń (co najmniej 42 pozycje), wskaźnik na
 * bufor w którym jest pakiet, rozmiar pakietu oraz wskanik na miejsce, gdzie
 * ma trafić numer gry.
 * Zwraca: liczbę wczytanych wydarzeń.
 */
size_t event_unserialise(struct event *events, void const *buffer, 
    size_t const buf_size, uint32_t *game_id);

/*
 * Konwersja enum wydarzenia na wartość liczbową.
 */
uint8_t event_serialise_type(enum event_type_t e);
enum event_type_t event_unserialise_type(uint8_t e);

/*
 * Funkcja alokuje pamięć na wydarzenie i wypełnia je przekazanymi danymi.
 * Tablica players to tablica na nazwy graczy zakończone znakiem '\0'.
 * Zwraca NULL w przypadku niepowodzenia (i ustawia errno).
 */
struct event_data_new_game *event_data_new_game_create(uint32_t width,
    uint32_t height, uint8_t pllayer_num, char *players[PLAYER_MAX_NUMBER]);

struct event_data_player_eliminated *event_data_player_eliminated_create(
    uint8_t id);

struct event_data_pixel *event_data_pixel_create(uint8_t id, uint32_t x, 
    uint32_t y);


#endif /*SERVER_TO_CLIENT_H*/
