#include "../include/weather.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BACKLOG 15

int main(void) {

  Weather_Server _Server;
  weather_server_init(&_Server);

  while (1)
  {
    scheduler_work(SystemMonotonicMS());
  }
  return 0;
}
