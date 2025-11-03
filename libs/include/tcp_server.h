#ifndef __TCP_H__
#define __TCP_H__
#define _POSIX_C_SOURCE 200809L
#include "../include/tcp_client.h"
#define MAX_CLIENTS 15

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

} TCP_Server;

int tcp_server_init(TCP_Server *_Server, const char* _Port, int _Backlog);
int tcp_server_init_ptr(TCP_Server** _ServerPtr, const char* _Port, int _Backlog);
int tcp_server_accept(TCP_Server *_Server);
void tcp_server_dispose(TCP_Server *_Server);
void tcp_server_dispose_ptr(TCP_Server** _ServerPtr);

#endif /* __TCP_H__ */
