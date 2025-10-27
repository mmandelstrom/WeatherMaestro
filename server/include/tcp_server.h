#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__
#include <stdbool.h>
#include <stdint.h>
#include "http_client.h"

typedef enum {
  SERVER_STATE_INIT,
  SERVER_STATE_CONNECTING,
  SERVER_STATE_READING,
  SERVER_STATE_WRITING,
  SERVER_STATE_DISPOSING,
  SERVER_STATE_ERROR

} ServerState;

#include "../../core/include/tcp_client.h"

typedef struct {
  int fd;
  uint16_t port;
  int backlog;
  ServerState state;

  Http_Client http;

} TCP_server;

int TCP_server_init(TCP_server *_Server, uint16_t _Port, int _Backlog);

int TCP_server_accept(TCP_server *_Server);

void TCP_server_dispose(TCP_server *_Server);

int TCP_server_read(TCP_client *_Client, uint8_t *buf, int len);

int TCP_server_write(TCP_client *_Client, uint8_t *buf, int len);

#endif /* __TCP_SERVER_H__ */
