#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/*
 * Oblicza crc32 z danych przekazanych w input, które mają długość
 * len bajtów.
 */
uint32_t compute_crc(uint8_t const *input, size_t len);

