#include "../include/TCP_client.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int TCP_client_init(TCP_client *_Client, uint16_t _Port, const char *_IP) {
  if (_Client == NULL) {
    return -1;
  }
  _Client->fd = -1;
  _Client->port = _Port;
  strcpy(_Client->ip, _IP);
  _Client->isConnected = false;
  return 0;
}

int TCP_client_connect(TCP_client *_Client) {
  struct sockaddr_in address;
  socklen_t addresslen = sizeof(address);

  _Client->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_Client->fd < 0) {
    perror("socket");
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(_Client->port);

  if (inet_pton(AF_INET, _Client->ip, &address.sin_addr) == -1) {
    perror("pton");
    return -1;
  }

  if (connect(_Client->fd, (struct sockaddr *)&address, addresslen) < 0) {
    perror("connect");
    return -1;
  }

  return 0;
}
int TCP_client_read(TCP_client *_Client, uint8_t *buf, int len) {
  return recv(_Client->fd, buf, len, MSG_DONTWAIT);
}

int TCP_client_write(TCP_client *_Client, uint8_t *buf, int len) {
  return send(_Client->fd, buf, len, MSG_NOSIGNAL);
}

void TCP_client_dispose(TCP_client *_Client) {
  if (_Client == NULL) {
    return;
  }
  memset(_Client, 0, sizeof(TCP_client));
  _Client = NULL;
}
