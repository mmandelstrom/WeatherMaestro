#include "../../libs/include/TCP_server.h"
#include "../../libs/include/TCP_client.h"
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define BACKLOG 15

int main() {
  TCP_server server;
  TCP_client client;
  client.fd = -1;

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
    
    if (client.fd < 0 ) {
      int cfd = TCP_server_accept(&server);
      if (cfd >= 0) {
        client.fd = cfd;
      } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        perror("accept");
      }
    }
   
    if (client.fd >= 0) {
      ssize_t bytesread = TCP_server_read(&client, buf, sizeof(buf));
      if (bytesread > 0) {
        buf[bytesread] = '\0';
        printf("%s", buf);
      } else if(bytesread == 0) {
        close(client.fd);
        client.fd = -1;
        continue;
      } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        perror("recv");
        close(client.fd);
        client.fd = -1;
        continue;
      }

      ssize_t bytestowrite = TCP_server_write(&client, (uint8_t*)message, strlen(message));
      if (bytestowrite <= 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        perror("send");
        close(client.fd);
        client.fd = -1;
      }
    }
  }

  TCP_server_dispose(&server);

  return 0;
}
