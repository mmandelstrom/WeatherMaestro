#include "../include/weather/weather_server.h"
#include <signal.h>

volatile sig_atomic_t stop = 0;
Weather_Server Server;

void handle_sigint(int sig)
{
  printf("\r\nShutting down server gracefully...\r\n");
  weather_server_dispose(&Server);
  stop = 1;
}

int main(void) {

  scheduler_init();
  weather_server_init(&Server);

  signal(SIGINT, handle_sigint);

  while (!stop)
  {
    scheduler_work(SystemMonotonicMS());
  }
  return 0;
}
