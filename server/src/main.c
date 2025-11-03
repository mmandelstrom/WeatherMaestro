#include "../../libs/include/TCP_client.h"
#include "../../libs/include/TCP_server.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define BACKLOG 15

/* static void sleep_ms(int ms) {
  struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
  nanosleep(&ts, NULL);
} */

int main(void) {
  TCP_server server;
  if (TCP_server_init(&server, "8080", BACKLOG) < 0) {
    perror("TCP_server_init");
    return 1;
  }

  while (1) {
    int cfd = TCP_server_accept(&server);
    if (cfd < 0) {
      /*No client currently*/
      continue;
    }

    TCP_client client = {0};
    client.fd = cfd;

    int bytesRead = 0;
    int i;
    for (i = 0; i < 50; ++i) { /*500ms timeout*/
      bytesRead = TCP_client_read(&client);
      if (bytesRead != 0) break;
      /* sleep_ms(10); */
    }

    if (bytesRead > 0 && client.readData) {
      /*Print request in terminal*/
      printf("\n=== Incoming request (%d bytes) ===\n", bytesRead);
      printf("%.*s\n", bytesRead, client.readData);
      printf("===================================\n");
    } else if (bytesRead < 0) {
    
      if (client.readData) free(client.readData);
      close(client.fd);
      continue;
    }

  
    /*httpresponse*/
    const char *body = "Hello from C server!\n";
    char header[256];
    int header_len = snprintf(
      header, sizeof(header),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Content-Length: %zu\r\n"
      "Connection: close\r\n"
      "\r\n",
      strlen(body)
    );

    /*writedata with header and rsponse*/
    size_t resp_len = (size_t)header_len + strlen(body);
    client.writeData = (char*)malloc(resp_len);
    memcpy(client.writeData, header, (size_t)header_len);
    memcpy(client.writeData + header_len, body, strlen(body));

    TCP_client_write(&client, resp_len);

    if (client.readData)  free(client.readData);
    if (client.writeData) free(client.writeData);
    close(client.fd);
  }

  TCP_server_dispose(&server);
  return 0;
}

