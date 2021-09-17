#ifndef COMMON_H
#define COMMON_H
#define _GNU_SOURCE
#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <fcntl.h>

// Maksymalna długość nicku użytkownika.
#define PLAYER_NAME_MAX_LENGTH  20
// Maksymalna obsługiwana liczba graczy.
#define PLAYER_MAX_NUMBER       25

/*
 * Zmienna do zwracania błędów w moich funkcjach, gdy nie jest to możliwe
 * inaczej (np. gdy zwracana jest liczba).
 * Istnieje, bo errno powinno raczej być wykorzystywane przez funkcje systemowe.
 */
extern int my_errno;

/*
 * Inicjuje funkcję wywołującą free na zarejestrowanych wskaźnikach.
 * Wywołuje free w kolejności rejestracji.
 */
void exit_init(void);

// Rejestruje wskaźnik ptr do free przed zakończeniem programu.
// Zwraca true, jeśli się powiodło.
bool exit_register_free(void *ptr);

/*
 * Wypisuje informację o błędnym zakończeniu funkcji wraz z errno i przerywa
 * program z kodem EXIT_FAILURE.
 * Argument: nazwa funkcji, w której wystąpił błąd.
 */
void fatal_err(char const *func);

/*
 * Wypisuje informację o błędnym zakończeniu funkcji i przerywa program z 
 * kodem EXIT_FAILURE.
 * Argument: wiadomość do wyświetlenia po napisie "ERROR: ".
 */
void fatal(char const *mess);

/*
 * Konweruje wejściowy ciąg znaków str do unsigned short int. 
 * Jeśli wystąpiły znaki niebędące cyframi w systemie dziesiętnym lub 
 * reprezentowana przez nie liczba przekorczyła USHRT_MAX to zwraca 0 i ustawia
 * zmienną my_errno na EINVAL (tak samo jeśli strtoul zwróci błąd z
 * jakiegokolwiek powodu).
 * Wartość zwracana: liczba reprezentowana przez string str.
 */
unsigned short int strtousi(char const * restrict str);

/*
 * Konweruje wejściowy ciąg znaków str do uint32_t. 
 * Jeśli wystąpiły znaki niebędące cyframi w systemie dziesiętnym lub 
 * reprezentowana przez nie liczba przekorczyła UINT32_MAX to zwraca 0 i ustawia
 * zmienną my_errno na EINVAL (tak samo jeśli strtoull zwróci błąd z
 * jakiegokolwiek powodu).
 * Wartość zwracana: liczba reprezentowana przez string str.
 */
uint32_t strtou32(char const * restrict str);

/*
 * Funkcja sprawdzająca, czy nazwa użytkownika jest zgodna z wymaganiami.
 * Ustawia my_errno na EINVAL jeśli nie jest poprawna i zwraca false.
 * Zwraca true, jeśli nazwa użytkownika nie jest pusta. False wpp
 */
bool validate_player_name(char const * restrict str);


#endif /*COMMON_H*/
