#ifndef __TCP_H__
#define __TCP_H__

#define _POSIX_C_SOURCE 200809L /* This must be defined before netdb.h */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BACKLOG 15
#define MAX_CLIENTS 15


/* ******************************************************************* */
/* *************************** TCP CLIENT **************************** */
/* ******************************************************************* */

typedef enum {
  CLIENT_STATE_INIT, 
  CLIENT_STATE_CONNECTING,
  CLIENT_STATE_READING,
  CLIENT_STATE_WRITING,
  CLIENT_STATE_DISPOSING,
  CLIENT_STATE_ERROR
} ClientState;

typedef struct {
  int fd;
  char* readData; /*Allocated in TCP_Client_Read, free'd in dispose*/
  char* writeData; /*Allocated in TCP_Client_Read, free'd in dispose*/
} TCP_Client;


int tcp_client_init(TCP_Client* _Client, const char* _Host, const char* _Port);
int tcp_client_init_ptr(TCP_Client** _ClientPtr, const char* _Host, const char* _Port);
int tcp_client_read(TCP_Client* _Client);
int tcp_client_write(TCP_Client* _Client, size_t _Length);
void tcp_client_dispose(TCP_Client* _Client);
void tcp_client_dispose_ptr(TCP_Client** _ClientPtr);


/* ******************************************************************* */
/* *************************** TCP SERVER **************************** */
/* ******************************************************************* */

typedef enum {
  SERVER_STATE_INIT,
  SERVER_STATE_CONNECTING,
  SERVER_STATE_READING,
  SERVER_STATE_WRITING,
  SERVER_STATE_DISPOSING,
  SERVER_STATE_ERROR
} ServerState;

typedef int (*tcp_server_on_accept)(int _FD, void* _Context);

typedef struct {
  int fd;
  const char* port;
  ServerState state;
  tcp_server_on_accept on_accept;
} TCP_Server;


int tcp_server_init(TCP_Server *_Server, const char* _Port, tcp_server_on_accept _OnAccept);
int tcp_server_init_ptr(TCP_Server** _ServerPtr, const char* _Port, tcp_server_on_accept _OnAccept);
int tcp_server_accept(TCP_Server *_Server);
void tcp_server_dispose(TCP_Server *_Server);
void tcp_server_dispose_ptr(TCP_Server** _ServerPtr);

#endif /* __TCP_SERVER_H__ */
