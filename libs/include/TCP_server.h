#ifndef __TCP_H__
#define __TCP_H__
#define _POSIX_C_SOURCE 200809L
#include "../include/TCP_client.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  SERVER_STATE_INIT,
  SERVER_STATE_CONNECTING,
  SERVER_STATE_READING,
  SERVER_STATE_WRITING,
  SERVER_STATE_DISPOSING,
  SERVER_STATE_ERROR
} ServerState;


typedef struct {
  int fd;
  const char* port;
  int backlog;
  ServerState state;

} TCP_server;

int TCP_server_init(TCP_server *_Server, const char* _Port, int _Backlog);

int TCP_server_accept(TCP_server *_Server);

void TCP_server_dispose(TCP_server *_Server);
/* int TCP_server_read(TCP_client *_Client);
int TCP_server_write(TCP_client *_Client, size_t _Length); */

#endif /* __TCP_H__ */
