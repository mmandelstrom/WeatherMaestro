#include "../../include/tcp.h"
#include <stdio.h>

/* -----------------Internal Functions----------------- */

void tcp_server_taskwork(void* _Context, uint64_t _MonTime);

/* ---------------------------------------------------- */

int tcp_server_set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int tcp_server_init(TCP_Server* _Server, const char* _port, tcp_server_on_accept _on_accept, void* _context) {
  if (!_Server) {
    errno = EINVAL; /*Invalid argument*/
    return -1;
  }
  _Server->context = _context;
  _Server->on_accept = _on_accept; 
  _Server->context = _context;
  _Server->fd = -1;
  _Server->port = _port;

  struct addrinfo addresses;
  memset(&addresses, 0, sizeof(addresses));
  addresses.ai_family = AF_UNSPEC; /*Both IPV4 & IPV6*/
  addresses.ai_socktype = SOCK_STREAM; /*TCP*/
  addresses.ai_flags = AI_PASSIVE; /*All network interfaces*/

  struct addrinfo* res = NULL;
  int getAddressInfo = getaddrinfo(NULL, _Server->port, &addresses, &res);
  if (getAddressInfo != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getAddressInfo));
    errno = EIO; /*General I/O issue likely hostname or nameresolution failed*/
    return -1;
  }

  int fd = -1;
  
  for (struct addrinfo* addressInfo = res; addressInfo != NULL; addressInfo = addressInfo->ai_next) {
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

    if (listen(fd, BACKLOG) != 0) {
      perror("listen");
      close(fd);
      fd = -1;
      continue;
    }

    _Server->fd = fd;

    if (tcp_server_set_nonblocking(_Server->fd) < 0) {
      close(_Server->fd);
      _Server->fd = -1;
      fd = -1;
      continue;
    }

    printf("Server listening on port: %s\n", _Server->port);
    freeaddrinfo(res);
	  
    _Server->task = scheduler_create_task(_Server, tcp_server_taskwork);

    return 0;
  }
  
  /*No addresses found*/
  freeaddrinfo(res);
  errno = EADDRNOTAVAIL; /*NO address available to bind*/
  return -1;
}


int tcp_server_init_ptr(TCP_Server** _Server_Ptr, const char* _port, tcp_server_on_accept _on_accept, void* _context) {
  if (!_Server_Ptr) {
    errno = EINVAL;
    return -1;
  }
  TCP_Server* server = (TCP_Server*)malloc(sizeof(TCP_Server));
  if (!server) {
    errno = ENOMEM; /*Out of memory*/
    perror("malloc");
    return -1;
  }
  int result = tcp_server_init(server, _port, _on_accept, _context);
  if (result != 0) {
    /*tcp_server_init already set errno*/
    free(server);
    return -1;
  }
  
  *(_Server_Ptr) = server;

  return 0;
}

int tcp_server_accept(TCP_Server *_Server) {
  if (!_Server) {
    errno = EINVAL;
    return TCP_ACCEPT_FATAL_ERROR;
  }

  struct sockaddr_storage address; /*Works for both ipv4 & ipv6*/
  socklen_t addressLength = sizeof(address);

  /* int client_fd = accept(_Server->fd, (struct sockaddr*)&address, &addressLength); */
  int client_fd = accept(_Server->fd, (struct sockaddr*)&address, &addressLength);
  if (client_fd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return TCP_ACCEPT_NO_CONNECTION;
    }
    perror("accept");
    return TCP_ACCEPT_FATAL_ERROR; // No connection yet
  }

  if (tcp_server_set_nonblocking(client_fd) < 0) {
    perror("tcp_server_set_nonblocking");
    close(client_fd);
    return TCP_ACCEPT_FATAL_ERROR;
  }

  int result = _Server->on_accept(client_fd, _Server->context);
  if (result != 0) {
    close(client_fd);
    errno = EIO; /*Generic I/O error*/
    return TCP_ACCEPT_NO_CONNECTION;
  }

  return client_fd;
}

void tcp_server_taskwork(void* _Context, uint64_t _MonTime)
{
	TCP_Server* _Server = (TCP_Server*)_Context;

	tcp_server_accept(_Server);
}

void tcp_server_dispose(TCP_Server *_Server) {
  if (_Server == NULL) {
    return;
  }
  memset(_Server, 0, sizeof(TCP_Server));
  _Server = NULL;
  if (_Server->fd >= 0) {
    printf("Dispose FD %i CLOSED\n", _Server->fd);
    close(_Server->fd);
    _Server->fd = -1;
  }

  _Server->port = NULL;
  _Server->on_accept = NULL;
}

void tcp_server_dispose_ptr(TCP_Server** _ServerPtr) {
  if (_ServerPtr == NULL || *(_ServerPtr) == NULL) {
    return;
  }
  tcp_server_dispose(*(_ServerPtr));
  free(*(_ServerPtr));
  *(_ServerPtr) = NULL;
}

