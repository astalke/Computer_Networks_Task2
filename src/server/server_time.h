#ifndef SERVER_TIME_H
#define SERVER_TIME_H

#include "common/common.h"
#include "server/opts.h"

/*
 * Zwraca ile sekund i mikrosekund minęło od [past].
 * [past] musi być w przeszłości.
 * Ustawia errno w przypadku błędu.
 */
struct timeval server_time_diff(struct timeval const *past);

/*
 * Uruchamia zegar, który będzie co 1/ROUNDS_PER_SEC ms informował o końcu rundy
 * Wysyła pierwszy komunikat po 1 ms.
 */
void server_clock_start(int timr, uint32_t rps);



#endif /*SERVER_TIME_H*/
