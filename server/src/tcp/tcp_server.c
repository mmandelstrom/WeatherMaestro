#include "../../include/tcp.h"

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

int tcp_server_init(TCP_Server *_Server, const char *_Port, tcp_server_on_accept _OnAccept, void* _context) {
  if (!_Server) {
    return -1;
  }
  _Server->on_accept = _OnAccept; 
  _Server->context = _context;
  _Server->fd = -1;
  _Server->port = _Port;

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
  return -1;
}


int tcp_server_init_ptr(TCP_Server** _ServerPtr, const char* _Port, tcp_server_on_accept _OnAccept, void* _context) {
  if (!_ServerPtr) {
    return -1;
  }
  TCP_Server* server = (TCP_Server*)malloc(sizeof(TCP_Server));
  if (!server) {
    perror("malloc");
    return -2;
  }
  int result = tcp_server_init(server, _Port, _OnAccept, _context);
  if (result != 0) {
    free(server);
    return -3;
  }
  
  *(_ServerPtr) = server;

  return 0;
}


int tcp_server_accept(TCP_Server *_Server) {
  if (!_Server) {
    return -1;
  }

  struct sockaddr_storage address; /*Works for both ipv4 & ipv6*/
  socklen_t addressLength = sizeof(address);

  /* int client_fd = accept(_Server->fd, (struct sockaddr*)&address, &addressLength); */
  int client_fd = accept(_Server->fd, (struct sockaddr*)&address, &addressLength);
  if (client_fd < 0) {
    return -1; // No connection yet
  }

  tcp_server_set_nonblocking(client_fd);

  int result = _Server->on_accept(client_fd, _Server);
  if (result != 0)
    close(client_fd);

  return 0;
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
  close(_Server->fd);
  memset(_Server, 0, sizeof(TCP_Server));
  _Server = NULL;
  if (_Server->fd >= 0) {
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

