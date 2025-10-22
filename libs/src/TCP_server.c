#include "../include/TCP_server.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int TCP_server_init(TCP_server *_Server, uint16_t _Port, int _Backlog) {
  if (_Server == NULL) {
    return -1;
  }
  _Server->fd = -1;
  _Server->port = _Port;
  _Server->isConnected = false;
  _Server->backlog = _Backlog;

  _Server->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_Server->fd < 0) {
    perror("socket");
    return -1;
  }

  int opt = 1;

  if (setsockopt(_Server->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) !=
      0) {
    perror("setsockopt");
    return -1;
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(_Server->port);
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  return bind(_Server->fd, (struct sockaddr *)&address, sizeof(address));

  if (listen(_Server->fd, _Server->backlog) < 0) {
    perror("listen");
    return -1;
  }

  printf("Server listening on port: %d\n", _Server->port);

  return 0;
}

int TCP_server_accept(TCP_server *_Server) {
  if (_Server == NULL) {
    return -1;
  }
  struct sockaddr address;
  socklen_t addresslen = sizeof(address);
  return accept(_Server->fd, &address, &addresslen);
}

void TCP_server_dispose(TCP_server *_Server) {
  if (_Server == NULL) {
    return;
  }
  memset(_Server, 0, sizeof(TCP_server));
  _Server = NULL;
}
int TCP_server_read(TCP_server *_Server, char *buf, int len) {
  return recv(_Server->fd, buf, len, MSG_DONTWAIT);
}
int TCP_server_write(TCP_server *_Server, uint8_t *buf, int len) {
  return send(_Server->fd, buf, len, MSG_NOSIGNAL);
}
