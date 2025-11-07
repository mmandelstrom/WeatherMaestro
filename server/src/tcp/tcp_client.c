#include "../../include/tcp.h"

/*---------------------Internal functions------------------------------*/
int tcp_client_set_nonblocking(int fd);
/*---------------------------------------------------------------------*/

int tcp_client_set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}


int tcp_client_init(TCP_Client* _Client, const char* _Host, const char* _Port) {
  _Client->fd = -1;
  _Client->readData = NULL;
  _Client->writeData = NULL;

  struct addrinfo addresses;
  memset(&addresses, 0, sizeof(addresses));
  addresses.ai_family = AF_UNSPEC;      /* IPv4 & IPv6 */
  addresses.ai_socktype = SOCK_STREAM;  /* TCP */
  addresses.ai_protocol = IPPROTO_TCP;

  struct addrinfo* result = NULL;
  int rc = getaddrinfo(_Host, _Port, &addresses, &result);
  if (rc != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
    return -1;
  }

  int fd = -1;

  for (struct addrinfo *addr_info = result; addr_info; addr_info = addr_info->ai_next) {
    fd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (fd < 0) continue;

    if (tcp_client_set_nonblocking(fd) != 0) {
      close(fd); fd = -1;
      continue;
    }

    int cres = connect(fd, addr_info->ai_addr, addr_info->ai_addrlen);
    if (cres == 0) {
      /*connected*/
      break;
    }
    if (cres < 0 && errno == EINPROGRESS) {
      /* non-blocking connect pågår */
      break;
    }

    close(fd);
    fd = -1;
  }

  freeaddrinfo(result);

  if (fd < 0) {
    return -1;
  }

  _Client->fd = fd;
  return 0;
}

int tcp_client_init_ptr(TCP_Client** _ClientPtr, const char* _Host, const char* _Port) {
  if (!_ClientPtr) {
    return -1;
  }
  TCP_Client* client = (TCP_Client*)malloc(sizeof(TCP_Client));
  if (!client) {
    perror("malloc");
    return -2;
  }
  int result = tcp_client_init(client, _Host, _Port);
  if (result != 0) {
    free(client);
    return -3;
  }
  
  *(_ClientPtr) = client;

  return 0;
}

int tcp_client_read_simple(TCP_Client* _Client, uint8_t* _buf, int _buf_len) {
  return recv(_Client->fd, _buf, _buf_len, MSG_DONTWAIT);
}


int tcp_client_read(TCP_Client* _Client) {
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

int tcp_client_write(TCP_Client* _Client, size_t _Length) {
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

void tcp_client_dispose(TCP_Client* _Client) {
  if (_Client == NULL) {
    return;
  }
  if (_Client->fd >= 0) {
    close(_Client->fd);
    _Client->fd = -1;
  }
  if (_Client->readData != NULL) {
    free(_Client->readData);
    _Client->readData = NULL;
  }
  if (_Client->writeData != NULL) {
    free(_Client->writeData);
    _Client->writeData = NULL;
  }
}

void tcp_client_dispose_ptr(TCP_Client** _ClientPtr) {
  if (_ClientPtr == NULL || *(_ClientPtr) == NULL) {
    return;
  }
  tcp_client_dispose(*(_ClientPtr));
  free(*(_ClientPtr));
  *(_ClientPtr) = NULL;
}

