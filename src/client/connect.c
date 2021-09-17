#include "client/connect.h"

/*
 * Inicjuje gniazdo TCP klienta IPv6 lub IPv4.
 * Argument: parametry wywołania.
 * Przerywa program w przypadku błędu.
 * Zwraca gniazdo, do którego możemy pisać przy pomocy send i czytać recv.
 */
int client_ipv6v4_tcp_init(unsigned short int gui_port, char const *gui_name) { 
  char port[20];
  sprintf(port, "%d", gui_port);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *res;
  if (getaddrinfo(gui_name, port, &hints, &res)) {
    fatal_err(__func__);    
  }

  int sock = socket(res->ai_family, SOCK_STREAM | SOCK_NONBLOCK, 
      res->ai_protocol);
  if (sock < 0) {
    fatal_err(__func__);    
  }

  //http://www.unixguide.net/network/socketfaq/2.16.shtml
  int flag = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, 
       sizeof(int)) < 0) {
    fatal_err(__func__);    
 }

  if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
    if (errno != EINPROGRESS) {
      fatal_err(__func__);    
    }
  } 
  struct pollfd fds[] = {
    (struct pollfd){.fd = sock, .events = POLLOUT, .revents = 0}
  };

  if (poll(fds, 1, 6) < -1) {
    fatal("Unable to connect to GUI.\n");
  }
  if (!(fds[0].revents & POLLOUT)) {
    fatal("Unable to connect to GUI.\n");
  }

  freeaddrinfo(res);
  return sock;
}


/*
 * Inicjuje gniazdo UDP klienta IPv6 lub IPv4.
 * Argument: parametry wywołania.
 * Przerywa program w przypadku błędu.
 * Zwraca gniazdo, do którego możemy pisać przy pomocy send i czytać recv.
 */
int client_ipv6v4_udp_init(unsigned short int portno, char const *name) { 
  char port[20];
  sprintf(port, "%d", portno);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *res;
  if (getaddrinfo(name, port, &hints, &res)) {
    fatal_err(__func__);    
  }

  int sock = socket(res->ai_family, SOCK_DGRAM, res->ai_protocol);
  if (sock < 0) {
    fatal_err(__func__);    
  }

  if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
    fatal_err(__func__);    
  } 

  freeaddrinfo(res);
  return sock;
}


