#include "server/server_time.h"

struct timeval server_time_diff(struct timeval const *past) {
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0) {
    time_t s = now.tv_sec - past->tv_sec;
    suseconds_t us = now.tv_usec;
    if (now.tv_usec < past->tv_usec) {
      --s;
      us += 1000000;
    }
    us -= past->tv_usec;
    return (struct timeval){.tv_sec = s, .tv_usec = us};
  }
  // Errno ustawione.
  return now;
}

void server_clock_start(int timr, uint32_t rps) {
  // Struktura zawierająca czas jaki mamy czekać.
  struct timespec t, t2;
  // Liczba sekund.
  uint64_t num_of_sec = 1;
  // Niemal zawsze 0.
  t.tv_sec = 1 / rps;
  num_of_sec -= t.tv_sec;
  // W sekundzie jest 10^9 nanosekund.
  t.tv_nsec = (num_of_sec * 1000000000) / rps;
  t2.tv_sec = 0;
  t2.tv_nsec = 1000000;
  struct itimerspec new_value = {.it_interval = t, .it_value = t2};
  if (timerfd_settime(timr, 0, &new_value, NULL) < 0) {
    fatal_err(__func__);    
  }
  // Kasujemy zaległe pakiety (po to mamy 1 ms oczekiwania).
  uint64_t dummy;
  read(timr, &dummy, sizeof(dummy));
}

