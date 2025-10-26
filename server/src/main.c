#include "../../libs/include/TCP_client.h"
#include "../../libs/include/TCP_server.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 15

int main() {
  TCP_server server;
  TCP_client client;
  client.fd = -1;

  uint16_t port = 5555;
  char buf[512];
  char message[1024];

  if (TCP_server_init(&server, port, BACKLOG) != 0) {
    printf("Failed to initialize server\n");
    return -1;
  }

  printf("Init successful\n");
  const char *readPtr = buf;
  int length = sizeof(buf) - 1;

  while (1) {

    if (client.fd < 0) {
      int cfd = TCP_server_accept(&server);
      if (cfd >= 0) {
        client.fd = cfd;
        printf("New client connected FD: %d\n", client.fd);

      } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        perror("accept");
      }
    }

    if (client.fd >= 0) {

      ssize_t bytesread = TCP_server_read(&client, (uint8_t *)readPtr, length);

      if (bytesread == 0) {
        printf("Client disconnected FD: %d\n", client.fd);
        close(client.fd);
        client.fd = -1;
        readPtr = buf; /*If client is closed reset buffer so next client can
                          reuse it*/
        length = sizeof(buf) - 1;
        buf[0] = '\0';
        continue;
      }

      else if (bytesread > 0) {
        length -= bytesread;
        readPtr += bytesread;
      } else if (bytesread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
          continue;
        } else {
          perror("recv");
          close(client.fd);
          client.fd = -1;
          continue;
        }
      }

      buf[bytesread] = '\0';
      printf("Data recieved from client: %s\n", buf);
      int bytesToSend = snprintf(message, sizeof(message), "Message echoed from server: %s\n", buf);

      if (bytesToSend < 0) {
        perror("snprintf");
        break;
      }

      int totalBytesSent = 0;

      while (totalBytesSent < bytesToSend) {
        ssize_t bytesSent =
            TCP_server_write(&client, (uint8_t *)message + totalBytesSent, bytesToSend - totalBytesSent);
        if (bytesSent > 0) {
          totalBytesSent += bytesSent;
          continue;
        }

        if (bytesSent == 0) {
          break;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
          break;
        } else {
          perror("send");
          close(client.fd);
          client.fd = -1;
          break;
        }
      }
    }
  }

  close(server.fd);
  TCP_server_dispose(&server);

  return 0;
}
