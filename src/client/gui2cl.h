#ifndef GUI_TO_CL_H
#define GUI_TO_CL_H

#include "common/common.h"


// Rozmiar bufora danych z strumienia TCP.
#define TCP_BUFFERSIZE 4096
// Zamknięto połączenie
#define TCP_EOF (-1)
// Wystąpił błąd
#define TCP_ERROR (-2)
// Nie ma więcej danych
#define TCP_NO_DATA (-3)

enum gui_message {
  GUI_LEFT_KEY_DOWN,
  GUI_LEFT_KEY_UP,
  GUI_RIGHT_KEY_DOWN,
  GUI_RIGHT_KEY_UP, 
  GUI_INVALID  
};

bool tcp_read_message(int sock, enum gui_message *mes);

int tcp_getchar(int sock);
// Czyści bufor.
void tcp_flush(void);
/*
 * Przesyła zawartość [buf] długości [count] bajtów na gniazdo [fd].
 */
int write_all(int fd, void const *buf, size_t const count);


#endif /*GUI_TO_CL_H*/
