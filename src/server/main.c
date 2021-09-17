#include "common/common.h"
#include "server/opts.h"
#include "server/random.h"
#include "common/cl2serv.h"
#include "server/game.h"

// Deskryptory plików do zamknięcia na koniec programu.
int g_to_close[] = {-1, -1};

// Funkcja zamykająca zarejestrowane deskryptory.
static void atexit_close_files(void) {
  size_t n = sizeof(g_to_close) / sizeof(*g_to_close);
  for (size_t i = 0; i < n; ++i) {
    if (g_to_close[i] >= 0) close(g_to_close[i]);
    g_to_close[i] = -1;
  }
}

/*
 * Inicjuje gniazdo serwera IPv6.
 * Argument: parametry wywołania, korzystamy z portu.
 * Przerywa program w przypadku błędu.
 */
static int server_ipv6v4_init(struct params const *parameters) {
  int sock = socket(AF_INET6, SOCK_DGRAM, 0);
  if (sock < 0) {
    fatal_err(__func__);    
  }
  g_to_close[0] = sock;
  struct sockaddr_in6 sock_data;
  memset(&sock_data, 0, sizeof(sock_data));
  sock_data.sin6_family = AF_INET6;
  sock_data.sin6_port = htons(parameters->port);
  sock_data.sin6_addr = in6addr_any;
  if (bind(sock, (struct sockaddr *)&sock_data, sizeof(sock_data)) < 0) {
    fatal_err(__func__);    
  }
  return sock;
}

/*
 * Inicjuje zegar. 
 * Przerywa program w przypadku błędu.
 */
static int server_clock_create(void) {
  int result = timerfd_create(CLOCK_REALTIME, 0);
  if (result < 0) {
    fatal_err(__func__);    
  }
  g_to_close[1] = result;
  return result;
}


int main(int argc, char *argv[]) {
  exit_init();
  atexit(atexit_close_files);
  struct params const parameters = parameters_init_global(argc, argv);
  // Inicjowanie generatora liczb losowych.
  server_random_init(parameters.seed);
  // Gniazdo sieciowe słuchające komunikatów.
  int sock = server_ipv6v4_init(&parameters);
  // Deskryptor zegara.
  int timr = server_clock_create();
  // Tablica deskryptorów: odbieranie pakietów i czasomierz.
  struct pollfd fds[2];
  fds[0] = (struct pollfd){.fd = timr, .events = POLLIN, .revents = 0};
  fds[1] = (struct pollfd){.fd = sock, .events = POLLIN, .revents = 0};
  // Maksymalny rozmiar pakietu (z dodatkowym miejscem).
  size_t const client_max_packet_size = cl2serv_size() + 10;
  // Bufor na pakiety klientów.
  char buffer[client_max_packet_size];

  // Struktura stanu gry.
  struct game game = game_create(parameters.width, parameters.height, 
      parameters.turning_speed, parameters.rounds_per_sec, sock, timr);

  do {
    poll(fds, 2, -1);
    // Czasomierz informuje, że trzeba przetworzyć rundę.
    if (fds[0].revents & POLLIN) {
      uint64_t num_of_packets;
      read(timr, &num_of_packets, 8);
      game_handle_game(&game);
    // Serwer czeka na pakiety.
    } else if (fds[1].revents & POLLIN) {
      // Potrzebne struktury.
      struct sockaddr_in6 client_address;
      socklen_t rcva_len = (socklen_t) sizeof(client_address);

      // Czyścimy bufor na pakiet.
      memset(buffer, 0, client_max_packet_size);

      ssize_t mes_len = recvfrom(fds[1].fd, buffer, client_max_packet_size, 0,
          (struct sockaddr *) &client_address, &rcva_len);
      
      // Sprawdzamy, czy otrzymaliśmy jakiekolwiek dane.
      if (mes_len > 0) {
        my_errno = 0;
        struct cl2serv data = cl2serv_deserialise(buffer, mes_len);
        // Czy udało się przetwozyć dane?
        if (my_errno == 0) {
          game_handle_packet(&game, &data, &client_address);
        } else {
          //Pakiet jest niepoprawny - ignorujemy go.
        }
      } else {
        // Wystąpił błąd lub jakimś cudem otrzymaliśmy pakiet rozłączenia.
        if (mes_len < 0) {
          fatal_err(__func__);
        }        
      }
    }
  } while(true);
}







