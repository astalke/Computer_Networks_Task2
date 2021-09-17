#include "common/cl2serv.h"

// Struktura pakietu przesyłanego z klienta do serwera.
struct __attribute__((packed)) cl2serv_packet {
  uint64_t session_id;
  uint8_t  turn_direction; // 0 - prosto, 1 - w prawo, wartość 2 - w lewo
  uint32_t next_expected_event_no;
  char player_name[PLAYER_NAME_MAX_LENGTH];
};

/*
 * Zwraca rozmiar metadanych pakietu cl2serv_packet.
 */
static size_t cl2serv_packet_meta_size(void) {
  return sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint32_t);
}

// Konwertuje turn_direction_t do liczby 8 bitowej.
static uint8_t direction_serialise(enum turn_direction_t dir) {
  switch (dir) {
    case DIRECTION_FORWARD:
      return 0;
      break;
    case DIRECTION_RIGHT:
      return 1;
      break;
    case DIRECTION_LEFT:
      return 2;      
      break;
    default:
      // Błąd programisty.
      assert(false);
      break;
  }
}

// Konwertuje liczbę 8 bitową do turn_direction_t.
static enum turn_direction_t direction_unserialise(uint8_t dir) {
  switch (dir) {
    case 0:
      return DIRECTION_FORWARD;
      break;
    case 1:
      return DIRECTION_RIGHT;
      break;
    case 2:
      return DIRECTION_LEFT;      
      break;
    default:
      return DIRECTION_INVALID;
      break;
  }
}

struct cl2serv cl2serv_deserialise(void const *buf, ssize_t size) { 
  struct cl2serv_packet const *alias_buf = buf;
  struct cl2serv result;
  memset(&result, 0, sizeof(result)); 
  result.session_id = be64toh(alias_buf->session_id);
  result.turn_direction = direction_unserialise(alias_buf->turn_direction);
  // Nieznana wartość turn_direction => błędny pakiet
  if (result.turn_direction == DIRECTION_INVALID) {
    my_errno = EINVAL;
    return result;
  }
  result.next_expected_event_no = be32toh(alias_buf->next_expected_event_no);
  size -= cl2serv_packet_meta_size();

  if (size < 0 || size > PLAYER_NAME_MAX_LENGTH) {
    my_errno = EINVAL;
    return result;
  }
  /*
   * Niech kompilator to zoptymalizuje, ja się boję tykać wskaźnikami
   * upakowaną strukturę.
   */
  for (ssize_t i = 0; i < size; ++i) {
    result.player_name[i] = alias_buf->player_name[i];
    // Znak 0 w nazwie użytkownika to błąd.
    if (result.player_name[i] == '\0') {
      my_errno = EINVAL;
      return result;
    }  
  }
  result.player_name[size] = 0; // Kończący znak 0.
  result.player_name_length = size;
  my_errno = 0;
  // Ustawi my_errno na odpowiednią wartość.
  (void) validate_player_name(result.player_name);
  return result;
}


size_t cl2serv_serialise(struct cl2serv const* arg, void *buf) {
  struct cl2serv_packet *alias_buf = buf;
  size_t size = cl2serv_packet_meta_size() + arg->player_name_length;
  alias_buf->session_id = htobe64(arg->session_id);
  alias_buf->turn_direction = direction_serialise(arg->turn_direction);
  alias_buf->next_expected_event_no = htobe32(arg->next_expected_event_no);
  /*
   * Niech kompilator to zoptymalizuje, ja się boję tykać wskaźnikami
   * upakowaną strukturę.
   */
  for (size_t i = 0; i < arg->player_name_length; ++i) {
    alias_buf->player_name[i] = arg->player_name[i];
  }
  return size;
}

size_t cl2serv_size(void) {
  return sizeof(struct cl2serv_packet);
}
