#include "../include/TCP_server.h"
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "../../utils/include/utils.h"
#define MAX_CLIENTS 15

int TCP_server_set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int TCP_server_init(TCP_server *_Server, const char *_Port, int _Backlog) {
  if (!_Server) {
    return -1;
  }
  
  _Server->fd = -1;
  _Server->port = _Port;
  _Server->backlog = _Backlog;

  struct addrinfo addresses;
  memset(&addresses, 0, sizeof(addresses));
  addresses.ai_family = AF_UNSPEC; /*Both IPV4 & IPV6*/
  addresses.ai_socktype = SOCK_STREAM; /*TCP*/
  addresses.ai_flags = AI_PASSIVE; /*All network interfaces*/

  struct addrinfo *res = NULL;
  int getAddressInfo = getaddrinfo(NULL, _Server->port, &addresses, &res);
  if (getAddressInfo != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getAddressInfo));
    return -1;
  }

  int fd = -1;
  
  for (struct addrinfo *addressInfo = res; addressInfo != NULL; addressInfo = addressInfo->ai_next) {
    fd = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
    if (fd < 0) continue; /*Try next*/
    
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
      perror("setsockopt");
    }
  
    if (bind(fd, addressInfo->ai_addr, addressInfo->ai_addrlen) != 0) {
      perror("bind");
      close(fd);
      fd = -1;
      continue; /*try next*/
    }

    if (listen(fd, _Backlog) != 0) {
      perror("listen");
      close(fd);
      fd = -1;
      continue;
    }

    _Server->fd = fd;

    if (TCP_server_set_nonblocking(_Server->fd) < 0) {
      close(_Server->fd);
      _Server->fd = -1;
      fd = -1;
      continue;
    }

    printf("Server listening on port: %s\n", _Server->port);
    freeaddrinfo(res);

    return 0;
  }
  
  /*No addresses found*/
  freeaddrinfo(res);
  return -1;
}

int TCP_server_accept(TCP_server *_Server) {
  if (!_Server) {
    return -1;
  }

  struct sockaddr_storage address; /*Works for both ipv4 & ipv6*/
  socklen_t addressLength = sizeof(address);

  int client_fd = accept(_Server->fd, (struct sockaddr*)&address, &addressLength);
  if (client_fd < 0) {
    return -1;
  }

  (void)TCP_server_set_nonblocking(client_fd);

  return client_fd;
}

void TCP_server_dispose(TCP_server *_Server) {
  if (_Server == NULL) {
    return;
  }
  close(_Server->fd);
  memset(_Server, 0, sizeof(TCP_server));
  _Server = NULL;
  if (_Server->fd >= 0) {
    close(_Server->fd);
    _Server->fd = -1;
  }

  _Server->port = NULL;
  _Server->backlog = 0;
}

int TCP_server_read(TCP_client* _Client) {
  if (!_Client) {
    return -1;
  }

  if (_Client->fd < 0) {
    return -2;
  }

  if (_Client->readData) {
    free(_Client->readData);
    _Client->readData = NULL;
  }

  size_t capacity = 512;
  _Client->readData = (char*)malloc(capacity + 1);
  if (!_Client->readData) {
    perror("malloc");
    return -3;
  }
  ssize_t bytesRead;
  size_t usedSpace = 0;
  size_t spaceLeft = capacity;

  while(1) {

    if (usedSpace >= capacity) {
      size_t newCapacity = capacity * 2;
      char* tempBuffer = (char*)realloc(_Client->readData, newCapacity + 1);
      if (!tempBuffer) {
        free(_Client->readData);
        _Client->readData = NULL;
        perror("realloc");
        return -4;
      }
      capacity = newCapacity;
      _Client->readData = tempBuffer;
      spaceLeft = capacity - usedSpace;
    }

    bytesRead = recv(_Client->fd, _Client->readData + usedSpace, spaceLeft, 0);

    if (bytesRead < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN) {
        if (usedSpace == 0) {
          return 0;
        }

        _Client->readData[usedSpace] = '\0';
                
        return (int)usedSpace;
      }

      free(_Client->readData);
      _Client->readData = NULL;
      perror("recv");
      return -5;
    }

    if (bytesRead == 0) {
      _Client->readData[usedSpace] = '\0';
      return (int)usedSpace;
    }

    if (bytesRead > 0) {
      usedSpace += (size_t)bytesRead;
      spaceLeft = capacity - usedSpace;
    }
  }
}


int TCP_server_write(TCP_client* _Client, size_t _Length) {
  if (!_Client || _Client->fd < 0 || !_Client->writeData) {
    return -1;
  }
  
  size_t bytesLeft = _Length;
  size_t totalSent = 0;
  ssize_t bytesSent;
  char* message = _Client->writeData;

  while (bytesLeft > 0) {

      bytesSent = send(_Client->fd, message, bytesLeft, 0);

      if (bytesSent < 0) {
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN) {
          return totalSent;
        }

        perror("send");
        return -2; // fatal error
      }

      if (bytesSent == 0) {
        return (int)totalSent;
      }

      totalSent += bytesSent;
      bytesLeft -= bytesSent;
      message += bytesSent;
  }
    return (int)totalSent;
}
 
