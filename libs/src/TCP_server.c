#include "../include/TCP_server.h"
#include "../include/TCP_client.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../../utils/include/utils.h"

int TCP_server_set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int TCP_server_init(TCP_server *_Server, uint16_t _Port, int _Backlog) {
  if (_Server == NULL) {
    return -1;
  }
  _Server->fd = -1;
  _Server->port = _Port;
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
  if (bind(_Server->fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind");
    return -1;
  }

  if (listen(_Server->fd, _Server->backlog) < 0) {
    perror("listen");
    return -1;
  }

  if (TCP_server_set_nonblocking(_Server->fd) < 0) {
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
  close(_Server->fd);
  memset(_Server, 0, sizeof(TCP_server));
  _Server = NULL;
}

int TCP_server_work(TCP_client* _Client) {
  uint64_t now = SystemMonotonicMS();
  uint64_t timeout = now + 5000;
  size_t capacity = 128;
  int bytesRecieved;
  size_t usedSpace = 0;

  while (now < timeout) {
    now = SystemMonotonicMS();

    if (usedSpace >= capacity) {
      printf("Capacity before expansion: %zu\n", capacity);
      size_t newCapacity = capacity * 2;
      char *tempBuf = (char *)realloc(response, newCapacity + 1);
      if (tempBuf == NULL) {
        free(response);
        perror("realloc");
        return -1;
      }
      capacity = newCapacity;
      response = tempBuf;
      printf("Capacity after: %zu\n", capacity);
    }

    size_t spaceLeft = capacity - usedSpace;

    bytesRecieved =
        TCP_client_read(&client, (uint8_t *)response + usedSpace, spaceLeft);

    if (bytesRecieved > 0) {
      usedSpace += bytesRecieved;
      continue;
    } else if (bytesRecieved == 0) {
      break;
    }


  size_t capacity = 1;
  int bytesRecieved;
  size_t usedSpace = 0;
  char *response = (char *)malloc(capacity + 1);
  if (response == NULL) {
    perror("malloc");
    return -1;
  }

  now = SystemMonotonicMS();
  timeout = now + 5000;

  while (now < timeout) {
    now = SystemMonotonicMS();

    if (usedSpace >= capacity) {
      printf("Capacity before expansion: %zu\n", capacity);
      size_t newCapacity = capacity * 2;
      char *tempBuf = (char *)realloc(response, newCapacity + 1);
      if (tempBuf == NULL) {
        free(response);
        perror("realloc");
        return -1;
      }
      capacity = newCapacity;
      response = tempBuf;
      printf("Capacity after: %zu\n", capacity);
    }

    size_t spaceLeft = capacity - usedSpace;

    bytesRecieved =
        TCP_client_read(&client, (uint8_t *)response + usedSpace, spaceLeft);

    if (bytesRecieved > 0) {
      usedSpace += bytesRecieved;
      continue;
    } else if (bytesRecieved == 0) {
      break;
    }
  }

  if (bytesRecieved > 0) {
    printf("TIMEOUT ON READ!\r\n");
    return 1;
  }

  response[usedSpace] = '\0';
  printf("DATA: %s\n", response);
 
}

int TCP_server_read(TCP_client *_Client, uint8_t *buf, int len) {
  return recv(_Client->fd, buf, len, MSG_DONTWAIT);
}
int TCP_server_write(TCP_client *_Client, uint8_t *buf, int len) {
  return send(_Client->fd, buf, len, MSG_NOSIGNAL);
}
