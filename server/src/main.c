#include "../../libs/include/TCP_server.h"
#include <stdint.h>
#include <stdio.h>

#define BACKLOG 15

int main() {
  TCP_server server;
  uint16_t port = 5555;
  char buf[512];
  int len = 512;
  char *message = "Test message from the server\n";

  if (TCP_server_init(&server, port, BACKLOG) != 0) {
    printf("Failed to initialize server\n");
    return -1;
  }

  printf("Init successful\n");

  while (1) {
    if (TCP_server_accept(&server) < 0) {
      continue;
    }
    printf("Accept successful\n");

    if (TCP_server_read(&server, buf, len) == -1) {
      continue;
    }
    printf("Read: %s\n", buf);

    if (TCP_server_write(&server, (uint8_t *)message, len) == -1) {
      continue;
    }
    printf("Write succesful\n");
  }

  TCP_server_dispose(&server);

  return 0;
}
