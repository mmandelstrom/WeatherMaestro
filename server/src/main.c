#include "weather/weather_server.h"
#include <signal.h>



volatile sig_atomic_t stop = 0;
Weather_Server Server;

/* Graceful exit */
void handle_sigint(int sig)
{
  printf("\r\nShutting down server gracefully...\r\n");
  weather_server_dispose(&Server);

#ifdef CURL_GLOBAL_DEFAULT
  curl_global_cleanup(); 
#endif

  stop = 1;
}

int main(void) {

  scheduler_init();
  weather_server_init(&Server);

#ifdef CURL_GLOBAL_DEFAULT
  curl_global_init(CURL_GLOBAL_DEFAULT); // To avoid as little still reachable memory reallocs we only run this once, this should be removed when we switch to http_client
#endif

  signal(SIGINT, handle_sigint);

  while (!stop)
  {
    scheduler_work(SystemMonotonicMS());
  }

  handle_sigint(SIGINT);
  return 0;
}
