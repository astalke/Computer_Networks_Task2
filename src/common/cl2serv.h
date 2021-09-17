#ifndef CLIENT_TO_SERVER_H
#define CLIENT_TO_SERVER_H
#include "common/common.h"

/*
 * Dostępne wartości pola turn_direction..
 */
enum turn_direction_t {DIRECTION_FORWARD, DIRECTION_RIGHT, 
                       DIRECTION_LEFT,    DIRECTION_INVALID};

// Struktura powstała po zdekodowaniu pakietu mającego identyczne pola.
struct cl2serv {
  uint64_t session_id;
  uint32_t next_expected_event_no;
  //0 - prosto, 1 - w prawo, wartość 2 - w lewo
  enum turn_direction_t turn_direction; 
  size_t player_name_length;        
  char player_name[PLAYER_NAME_MAX_LENGTH + 1]; // Dodatkowy znak na bajt 0.
};

/*
 * Konstruuje obiekt typu struct cl2serv z czystych danych otrzymanych od
 * klienta, które są przekazane przez argument [buf].
 * Sprawdza poprawność nazwy użytkownika.
 * Argument [size] to rozmiar danych w bajtach.
 * Ustawia my_errno jeśli wystąpi błąd.
 */
struct cl2serv cl2serv_deserialise(void const *buf, ssize_t size);

/*
 * Z obiektu [arg] typu struct cl2serv tworzy pakiet danych do przesłania.
 * Umieszcza go pod adresem buf.
 * Zwraca rozmiar w bajtach pakietu.
 */
size_t cl2serv_serialise(struct cl2serv const* arg, void *buf);

/*
 * Zwraca rozmiar pakietu danych kodującego cl2serv.
 */
size_t cl2serv_size(void);


#endif /*CLIENT_TO_SERVER_H*/
