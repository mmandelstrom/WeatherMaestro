#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__
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

typedef enum {
  CLIENT_STATE_INIT, /*Should set to 0- whatever is the last number by default*/
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

#endif
