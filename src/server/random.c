#include "server/random.h"

static uint32_t s_seed = 0;
static uint64_t s_factor = 279410273;
static uint64_t s_modulo = 4294967291;

void server_random_init(uint32_t seed) {
  s_seed = seed;
}

uint32_t server_random_rand(void) {
  uint32_t result = s_seed;
  uint64_t temp = s_seed;
  temp *= s_factor;
  temp %= s_modulo;
  s_seed = (temp & 0xFFFFFFFF);
  return result;
}


