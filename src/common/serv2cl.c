#include "common/serv2cl.h"

// Metadane wydarzenia.
struct __attribute__((packed)) event_metadata_packet {
  uint32_t len;                 // Sumaryczna długość pól event_*
  uint32_t event_no;            // Numer wydarzenia.
  uint8_t event_type;           // Jakie wydarzenie. 
};

// Wydarzenie: rozpoczęcie nowej gry.
struct __attribute__((packed)) event_new_game_packet {
  uint32_t maxx;                // Szerokość planszy w pikselach.
  uint32_t maxy;                // Wysokość planszy w pikselach.
  // Tablica nazw graczy - jest w domyśle.
  //char players[PLAYER_MAX_NUMBER][PLAYER_NAME_MAX_LENGTH];
  uint32_t crc32;
};

// Wydarzenie: pixel
struct __attribute__((packed)) event_pixel_packet {
  uint8_t player_number;        // Numer gracza.
  uint32_t x;                   // Odcięta
  uint32_t y;                   // Rzędna.
  uint32_t crc32;
};

// Wydarzenie: PLAYER_ELIMINATED
struct __attribute__((packed)) event_player_eliminated_packet {
  uint8_t player_number;        // Numer gracza. 
  uint32_t crc32;
};

// Wydarzenie: GAME_OVER
struct __attribute__((packed)) event_game_over_packet {
  uint32_t crc32;
};

// Zwraca rozmiar zapakowanej struktury nowej gry.
static size_t event_size_new_game(struct event const *arg) {
  size_t result = sizeof(struct event_new_game_packet);
  struct event_data_new_game const *ptr = arg->event_data;
  /*
   * Teraz trzeba obliczyć długości nazw graczy. Każdy z nich zajmuje
   * długość swojej nazwy gracza + 1 (znak \0).
   */
  for (size_t i = 0; i < ptr->player_num; ++i) {
    result += 1 + strlen(ptr->players[i]);
  }
  return result;
}

/*
 * Zwraca rozmiar w bajtach zapakowanej struktury.
 */
static size_t event_size(struct event const *arg) {
  size_t result = sizeof(struct event_metadata_packet);
  switch(arg->event_type) {
    case NEW_GAME:
      result += event_size_new_game(arg);
      break;
    case PIXEL:
      result += sizeof(struct event_pixel_packet);
      break;
    case PLAYER_ELIMINATED:
      result += sizeof(struct event_player_eliminated_packet);
      break;
    case GAME_OVER:
      result += sizeof(struct event_game_over_packet);
      break;
    default:
      assert(false);
      break;
  }
  return result;
}

static void event_serialise_new_game(struct event_data_new_game const *arg, 
    void * const restrict buf, size_t offset) {
  struct event_new_game_packet packet;
  packet.maxx = htobe32(arg->maxx);
  packet.maxy = htobe32(arg->maxy);
  size_t t_offset = sizeof(packet) - sizeof(uint32_t);/*crc*/
  (void) memcpy(buf + offset, &packet, t_offset);
  offset += sizeof(struct event_new_game_packet) - sizeof(uint32_t); /*crc*/
  for (uint8_t i = 0; i < arg->player_num; ++i) {
    char const *name = arg->players[i];
    size_t length = strnlen(name, PLAYER_NAME_MAX_LENGTH);
    (void) strncpy(buf + offset, name, length);
    offset += length;
    // Znak rozdzielający nazwy graczy.
    ((char *)buf)[offset] = '\0';
    ++offset;
  }
  uint32_t crc = htobe32(compute_crc((uint8_t const *)buf, offset));
  (void) memcpy(buf + offset, &crc, sizeof(crc));  
}

static void event_serialise_pixel(struct event_data_pixel const *arg,
    void * const restrict buf, size_t offset) {
  struct event_pixel_packet packet;
  packet.player_number = arg->player_number;
  packet.x = htobe32(arg->x);
  packet.y = htobe32(arg->y);
  size_t t_offset = sizeof(packet) - sizeof(uint32_t);/*crc*/
  (void) memcpy(buf + offset, &packet, t_offset);
  offset += t_offset;
  uint32_t crc = htobe32(compute_crc((uint8_t const *)buf, offset));
  (void) memcpy(buf + offset, &crc, sizeof(crc));  
}

static void event_serialise_player_eliminated(
    struct event_data_player_eliminated const *arg,
    void * const restrict buf, size_t offset) {
  struct event_player_eliminated_packet packet;
  packet.player_number = arg->player_number;
  size_t t_offset = sizeof(packet) - sizeof(uint32_t);/*crc*/
  (void) memcpy(buf + offset, &packet, t_offset);
  offset += t_offset;
  uint32_t crc = htobe32(compute_crc((uint8_t const *)buf, offset));
  (void) memcpy(buf + offset, &crc, sizeof(crc));  
}

static void event_serialise_game_over(void * const restrict buf, 
    size_t offset) {
  uint32_t crc = htobe32(compute_crc((uint8_t const *)buf, offset));
  (void) memcpy(buf + offset, &crc, sizeof(crc));  
}

void event_destroy(void *event) {
  if (event != NULL) {
    struct event *e = event;
    // Możemy wywołać free na NULL bez konsekwencji.
    free(e->event_data);
  }
}

struct event event_create(uint32_t event_no, enum event_type_t event_type,
    void *event_data) {
  struct event result = {.event_data = event_data, .event_no = event_no, 
    .event_type = event_type};
  return result;
}

uint8_t event_serialise_type(enum event_type_t e) {
  switch(e) {
    case NEW_GAME:
      return 0;
      break;
    case PIXEL:
      return 1;
      break;
    case PLAYER_ELIMINATED:
      return 2;
      break;
    case GAME_OVER:
      return 3;
      break;
    default:
      assert(false);
      break;
  }
}

enum event_type_t event_unserialise_type(uint8_t e) {
  switch(e) {
    case 0:
      return NEW_GAME;
      break;
    case 1:
      return PIXEL;
      break;
    case 2:
      return PLAYER_ELIMINATED;
      break;
    case 3:
      return GAME_OVER;
      break;
    default:
      return EV_INVALID;
      break;
  }
}

static bool event_unserialise_new_game(struct event *event, char const *data,
    size_t const size) {
  struct event_data_new_game_client *result = malloc(sizeof(*result));
  if (result == NULL) { 
    fatal_err(__func__);
  }
  memset(result, 0, sizeof(*result));

  size_t offset = 0;
  memcpy(&result->maxx, data + offset, sizeof(result->maxx));
  assert(sizeof(result->maxx) == sizeof(uint32_t));
  result->maxx = be32toh(result->maxx);
  offset += sizeof(result->maxx);
  memcpy(&result->maxy, data + offset, sizeof(result->maxy));
  assert(sizeof(result->maxy) == sizeof(uint32_t));
  result->maxy = be32toh(result->maxy);
  offset += sizeof(result->maxy); 

  size_t t_offset = offset;
  // t_offset teraz wskazuje na pierwszy bajt nazwy gracza.
  while(t_offset < size) {
    // Obliczamy długość nazwy gracza.
    size_t p_len;
    char const *s = data + t_offset;
    for (p_len = 0; s[p_len]; ++p_len) {
      char c = s[p_len];
      // Niepoprawne znaki w nazwie
      if (c < 33 || c > 126) {
        return false;
      }
    }
    // Za długa nazwa użytkownika - bezsensowne dane.
    if (p_len > PLAYER_NAME_MAX_LENGTH) return false;
    // Dane są poprawne.
    t_offset += p_len + 1;
  }
  memcpy(result->players, data + offset, size - offset);  
  result->players_length = size - offset;
  event->event_data = result;
  if (event->event_no != 0) {
    return false;
  }
  return true;
}

static bool event_unserialise_pixel(struct event *event, 
    struct event_pixel_packet const *data, size_t const size) {
  struct event_data_pixel *result;
  // Otrzymana długość danych nie zgadza się z rzeczywistością.
  if (size + sizeof(uint32_t) != sizeof(*data)) {
    //printf("Hi: %zd %zd\n", size + sizeof(uint32_t), sizeof(*data));
    return false;
  }
  result = malloc(sizeof(*result));
  if (result == NULL) {
    fatal_err(__func__);
  }
  result->player_number = data->player_number;
  result->x = be32toh(data->x);
  result->y = be32toh(data->y);                 
  event->event_data = result;
  return true;
}

static bool event_unserialise_player_eliminated(struct event *event, 
    struct event_player_eliminated_packet const *data, size_t const size) {
  // Otrzymana długość danych nie zgadza się z rzeczywistością.
  if (size + sizeof(uint32_t) != sizeof(*data)) {
    return false;
  }

  struct event_data_player_eliminated *result;
  result = malloc(sizeof(*result));
  if (result == NULL) {
    fatal_err(__func__);
  }
  result->player_number = data->player_number;
  event->event_data = result;
  return true;
}

// data to wskaźnik na początek danych, gdzie znajdują się dane pakietu
static bool event_unserialise_event_data(struct event *event, void const *data,
    size_t const size) {
  switch(event->event_type) {
    case NEW_GAME:
      return event_unserialise_new_game(event, data, size);
      break;
    case PIXEL:
      return event_unserialise_pixel(event, data, size);
      break;
    case PLAYER_ELIMINATED:
      return event_unserialise_player_eliminated(event, data, size);
      break;
    case GAME_OVER:
      event->event_data = NULL;
      break;
    default:
      // Nieznane - nic nie robimy.
      break;
  }
  return true;
}

// Próbuje odczytać wydarzenie. Zwraca false jeśli się nie udało.
// Jeśli się uda, to przesuwa *[offset].
// Ustawia bajty w kolejności hosta.
static bool event_unserialise_one(struct event *event, void const *buffer, 
    size_t const buf_size, size_t *offset) {
  size_t const coffset = *offset;
  struct event_metadata_packet meta;
  //printf("offset: %zd\n", *offset);
  if (*offset + sizeof(meta) <= buf_size) {
    memcpy(&meta, buffer + *offset, sizeof(meta));
    *offset += sizeof(meta);

    uint32_t len = be32toh(meta.len);
    uint32_t const clen = len;
    event->event_no = be32toh(meta.event_no);
    event->event_type = event_unserialise_type(meta.event_type);
    event->event_data = NULL;

    // Sprawdzamy, czy pakiet przekazał wystarczająco duże len.
    if (len < sizeof(uint32_t) + sizeof(uint8_t)) {
      // Uszkodzony pakiet, niemożliwe odczytanie crc32
      fatal("Damaged packet.");
      return false;
    }
    // Otrzymujemy długość pola event_data
    len -= sizeof(uint32_t) + sizeof(uint8_t);

    // Czy w pakiecie jest dość danych?
    if (*offset + len <= buf_size) {
      // Próbujemy uzupełnić dane.
      bool valid = event_unserialise_event_data(event, buffer + *offset, len);
      *offset += len;
      // Nawet jeśli nie jest valid, to sprawdzamy crc32
      uint32_t received_crc32;
      memcpy(&received_crc32, buffer + *offset, sizeof(received_crc32));
      *offset += sizeof(received_crc32);
      received_crc32 = be32toh(received_crc32);
      uint32_t expected_crc32 = compute_crc(buffer + coffset, clen + sizeof(len));
      if (received_crc32 != expected_crc32) {
        // Uszkodzony pakiet - niepoprawna suma kontrolna.
        return false;
      }
      if (valid) {
        // Dane poprawne
        return true;
      }
      // Poprawne CRC32, dane bez sensu
      fatal("Correct CRC32, but data corrupted\n");
    }
  }
  /*
   * Uszkodzony pakiet, niemożliwe odczytanie CRC32.
   */
  fatal("Damaged packet.");
  return false;
}

size_t event_unserialise(struct event *events, void const *buffer, 
  size_t const buf_size, uint32_t *game_id) {
  size_t result = 0;
  // Czy pakiet zawiera dość danych?
  if (buf_size < sizeof(*game_id)) {
    my_errno = EINVAL;
    return 0;
  }
  size_t offset = 0;
  memcpy(game_id, buffer + offset, sizeof(*game_id));
  *game_id = be32toh(*game_id);
  offset += sizeof(*game_id);
  while (offset < buf_size) {
    struct event event;
    if (event_unserialise_one(&event, buffer, buf_size, &offset)) {
      events[result++] = event;
      continue;
    } 
    // Jeśli się uda odczytać całość, wyżej powinno zostać wywołane continue.
    break;
  }
  return result;
}

bool event_serialise(struct event const *arg, void * const restrict buf, 
    size_t *buf_size) {
  size_t size = event_size(arg);
  size_t offset = 0;
  if (size <= *buf_size) {
    *buf_size -= size;
    // Metadane pakietu.
    struct event_metadata_packet meta;
    meta.len = htobe32((uint32_t)size - 2 * sizeof(uint32_t)); /*len i crc*/
    meta.event_no = htobe32(arg->event_no);
    meta.event_type = event_serialise_type(arg->event_type);
    (void) memcpy(buf + offset, &meta, sizeof(meta));
    offset += sizeof(meta);
    switch (arg->event_type) {
      case NEW_GAME:
        event_serialise_new_game(arg->event_data, buf, offset);
        break;
      case PIXEL:
        event_serialise_pixel(arg->event_data, buf, offset);
        break;
      case PLAYER_ELIMINATED:
        event_serialise_player_eliminated(arg->event_data, buf, offset);
        break;
      case GAME_OVER:
        event_serialise_game_over(buf, offset);
        break;
      default:
        assert(false);
        break;
    }
    return true;    
  }
  return false;
}

struct event_data_new_game *event_data_new_game_create(uint32_t width,
    uint32_t height, uint8_t player_num, char *players[PLAYER_MAX_NUMBER]) {
  assert(player_num <= PLAYER_MAX_NUMBER);
  assert(players != NULL);
  assert(width > 0);
  assert(height > 0);

  struct event_data_new_game *result = malloc(sizeof(*result));
  if (result != NULL) {
    result->maxx = width;
    result->maxy = height;
    result->player_num = player_num;
    for (uint8_t i = 0; i < player_num; ++i) {
      strncpy(result->players[i], players[i], PLAYER_MAX_NUMBER);
    }
  }
  return result;
}

struct event_data_player_eliminated *event_data_player_eliminated_create(
    uint8_t id) {
  struct event_data_player_eliminated *result = malloc(sizeof(*result));
  if (result != NULL) {
    result->player_number = id;
  }
  return result;
}

struct event_data_pixel *event_data_pixel_create(uint8_t id, uint32_t x, 
    uint32_t y) {
  struct event_data_pixel *result = malloc(sizeof(*result));
  if (result != NULL) {
    result->player_number = id;
    result->x = x;
    result->y = y;
  }
  return result;
}
