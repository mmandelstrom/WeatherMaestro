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

