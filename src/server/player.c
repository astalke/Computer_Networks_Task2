#include "player.h"

static uint64_t const hash_modulo = 1000696969;
static uint64_t const hash_base = 101;

static int player_compare(struct player const *first, 
    struct player const *second) {
  return strncmp(first->player_name, second->player_name, 
      PLAYER_NAME_MAX_LENGTH);
}

static void calculate_hash(struct player *p) {
  uint64_t result = 0;
  for (size_t i = 0; i < p->player_name_length; ++i) {
    uint8_t c = p->player_name[i] - 32;
    result = (result * hash_base) % hash_modulo;
    result = (result + c) % hash_modulo;
  }
  p->hash = result;
}

void player_touch(struct player *player) { 
  // Funkcja ustawi errno.
  (void) gettimeofday(&player->last_active, NULL);
}

bool player_is_inactive(struct player *player) {
  struct timeval result = server_time_diff(&player->last_active);
  return (result.tv_sec >= 2);
}

struct player player_create(struct cl2serv const *data, 
                            struct sockaddr_in6 const * restrict sock_data,
                            enum player_queue queue, uint8_t id) {

  struct player result;
  // x i y ustawiane dopiero po rozpoczęciu gry.
  result.session_id = data->session_id;
  result.player_name_length = data->player_name_length;
  result.queue = queue;
  result.flags = PLAYER_RECEIVER;
  result.id = id;
  result.next_move = DIRECTION_FORWARD;
  memcpy(&result.sock_data, sock_data, sizeof(struct sockaddr_in6));
  errno = 0;
  player_touch(&result);
  if (errno == 0) {
    (void) memcpy(result.player_name, data->player_name, 
        data->player_name_length);
    result.player_name[result.player_name_length] = 0;
  }
  calculate_hash(&result);
  return result;
}

void player_destroy(struct player *p) {
  (void)p;
}

int player_compare_for_qsort(void const *first, void const *second) {
  return player_compare(first, second);
}

bool player_move(struct player *p) {
  uint32_t oldx = (uint32_t)p->x, oldy = (uint32_t)p->y;
  double angle = (double)p->direction;
  // Konwersja na radiany.
  angle *= M_PI;
  angle /= 180.0;
  // Domyślnie jest przeciwnie do ruchu wskazówek zegara.
  angle *= -1.0;
  // Macierz obrotu * [1, 0] = [cos(angle), sin(angle)]
  // Ale skoro y rośnie w dół, to trzeba odjąć dy
  double dx = cos(angle);
  double dy = sin(angle);
  p->x += dx;
  p->y -= dy;
  uint32_t newx = (uint32_t)p->x, newy = (uint32_t)p->y;
  return ((newx != oldx) || (newy != oldy));
}
