/*
 * Implementacja generatora liczb "losowych".
 */
#ifndef SERVER_RANDOM_H
#define SERVER_RANDOM_H

#include <stdint.h>

// Funkcja inicjująca generator wartością seed.
void server_random_init(uint32_t seed);

// Funkcja zwracająca kolejną liczbę pseudolosową.
uint32_t server_random_rand(void);

#endif /*SERVER_RANDOM_H*/
