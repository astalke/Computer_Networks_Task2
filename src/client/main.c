#include "common/common.h"
#include "client/opts.h"
#include "client/game.h"
#include "common/serv2cl.h"

struct game game;
bool game_init = false;

static void game_close(void) {
  if (game_init) {
    game_destroy(&game);
  }
}

int main(int argc, char *argv[]) {  
  if (atexit(game_close)) {
    fatal("Unable to register function!\n");
  }
  game_init = false;
  // Inicjacja struktur sprzątających.
  exit_init();
  struct params const params = parameters_init_global(argc, argv);
  game = game_create(&params);
  game_init = true;
  
  // Tablica deskryptorów: komunikacja z serwerem gry, czasomierz.
  struct pollfd fds[] = {
    (struct pollfd){.fd = game.timr, .events = POLLIN, .revents = 0},
    (struct pollfd){.fd = game.server_sock, .events = POLLIN, .revents = 0},
    (struct pollfd){.fd = game.gui_sock, .events = POLLIN, .revents = 0}
  };
  size_t const fds_len = sizeof(fds) / sizeof(*fds);
  
  do {
    poll(fds, fds_len, -1);
    if (fds[0].revents & POLLIN) {
      uint64_t num_of_packets;
      read(game.timr, &num_of_packets, 8);
      game_send_status(&game);
    } else if (fds[1].revents & POLLIN) {
      game_handle_packet(&game);
    } else if (fds[2].revents & POLLIN) {
      game_handle_gui(&game);
    }
  } while(true);
}
