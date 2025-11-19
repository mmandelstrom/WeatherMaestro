#include "../include/weather/weather_server.h"


int main(void) {

  scheduler_init();

  Weather_Server _Server;
  weather_server_init(&_Server);

  while (1)
  {
    scheduler_work(SystemMonotonicMS());
  }
  return 0;
}
