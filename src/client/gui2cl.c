#include "client/gui2cl.h"

// Bufor odebranych danych.
static uint8_t tcp_buffer[TCP_BUFFERSIZE + 1];
// Aktualny rozmiar bufora.
static ssize_t tcp_buffer_length;
// Wskazuje na pierwszy nieprzeczytany znak.
static ssize_t tcp_buffer_counter;

bool tcp_read_message(int sock, enum gui_message *mes) {
  static char buffer[TCP_BUFFERSIZE];
  static size_t buffer_len = 0;
  static bool is_valid = true;
  int c = 0;
  while ((c != '\n') && (c != -1)) {
    c = tcp_getchar(sock);
    if ((c != '\n') && (c != -1) && is_valid) {
      buffer[buffer_len++] = c;      
      // Za długi pakiet.
      if (buffer_len > 14) {
        is_valid = false;
        buffer_len = 0;
      }
    }
  }
  // Koniec pakietu.
  if (c == '\n') {
    if (is_valid) {
      buffer[buffer_len++] = 0;
      bool result = false;
      if (strcmp(buffer, "LEFT_KEY_DOWN") == 0) {
        *mes = GUI_LEFT_KEY_DOWN;
        result = true;
      } else if (strcmp(buffer, "LEFT_KEY_UP") == 0) {
        *mes = GUI_LEFT_KEY_UP;
        result = true;
      } else if (strcmp(buffer, "RIGHT_KEY_DOWN") == 0) {
        *mes = GUI_RIGHT_KEY_DOWN;
        result = true;
      } else if (strcmp(buffer, "RIGHT_KEY_UP") == 0) {
        *mes = GUI_RIGHT_KEY_UP;
        result = true;
      }
      buffer_len = 0;
      return result;
    } else {
      // Nowy pakiet zaczyna jako poprawny.
      is_valid = true;
    }
  }
  return false;
}

int tcp_getchar(int sock) {
  int result = 0;
  // Wciąż są wolne dane w buforze.
  if (tcp_buffer_counter < tcp_buffer_length) {
    result = tcp_buffer[tcp_buffer_counter++];
    return result;
  } 
  // Nie ma już danych w buforze - ciągniemy je.
  ssize_t ret = recv(sock, tcp_buffer, TCP_BUFFERSIZE, 0);
  // Czy zamknięto połączenie?
  if (ret == 0) {
    fatal("Lost connection to GUI\n");
  } else if (ret < 0) {
    // Odczyt byłby blokujący.
    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
      my_errno = TCP_NO_DATA;
      return -1;
    } else {
      fatal("Error in connection to GUI\n");
    }
  }
  // Wszystko w porządku - przeczytaliśmy co najmniej 1 bajt.
  tcp_buffer_length = ret;
  tcp_buffer_counter = 0;
  result = tcp_buffer[tcp_buffer_counter++];
  return result;
}

void tcp_flush(void) {
  tcp_buffer_length = 0;
  tcp_buffer_counter = 0;
}

int write_all(int fd, void const *buf, size_t const count) {
  size_t written = 0;
  while (written < count) {
    ssize_t ret = send(fd, buf + written, count - written, MSG_NOSIGNAL);
    if (ret < 0) {
      // Klient zamknął połączenie w czasie pisania lub błąd.
      // Zachowujemy errno.
      return -1;
    }
    written += ret;
  }
  return 0;
}


