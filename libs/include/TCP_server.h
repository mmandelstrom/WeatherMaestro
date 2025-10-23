#ifndef __TCP_H__
#define __TCP_H__
#include <stdbool.h>
#include <stdint.h>
#include "../include/TCP_client.h"

typedef struct {
  int fd;
  uint16_t port;
  bool isConnected;
  int backlog;

} TCP_server;

int TCP_server_init(TCP_server *_Server, uint16_t _Port, int _Backlog);

int TCP_server_accept(TCP_server *_Server);

void TCP_server_dispose(TCP_server *_Server);
int TCP_server_read(TCP_client *_Client, char *buf, int len);
int TCP_server_write(TCP_client *_Client, uint8_t *buf, int len);

#endif /* __TCP_H__ */
