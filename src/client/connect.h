#ifndef CLIENT_CONNECT_H
#define CLIENT_CONNECT_H

#include "common/common.h"

/*
 * Inicjuje gniazdo UDP klienta IPv6 lub IPv4.
 * Argument: parametry wywołania.
 * Przerywa program w przypadku błędu.
 * Zwraca gniazdo, do którego możemy pisać przy pomocy send i czytać recv.
 */
int client_ipv6v4_udp_init(unsigned short int portno, char const *name); 

/*
 * Inicjuje gniazdo TCP klienta IPv6 lub IPv4.
 * Argument: parametry wywołania.
 * Przerywa program w przypadku błędu.
 * Zwraca gniazdo, do którego możemy pisać przy pomocy send i czytać recv.
 */
int client_ipv6v4_tcp_init(unsigned short int gui_port, char const *gui_name); 



#endif /*CLIENT_CONNECT_H*/
