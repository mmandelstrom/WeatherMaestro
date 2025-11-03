#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__
#include <stdbool.h>
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
  char* readData; /*Allocated in TCP_client_Read, free'd in dispose*/
  char* writeData; /*Allocated in TCP_client_Read, free'd in dispose*/
} TCP_client;


int TCP_client_Initiate(TCP_client* _Client, const char* _Host, const char* _Port);
int TCP_client_InitiatePtr(TCP_client** _ClientPtr, const char* _Host, const char* _Port);
int TCP_client_read(TCP_client* _Client);
int TCP_client_write(TCP_client* _Client, size_t _Length);
void TCP_client_Dispose(TCP_client* _Client);
void TCP_client_DisposePtr(TCP_client** _ClientPtr);


/*
typedef struct {
  int fd;
  uint16_t port;
  char* host;
  char* readData;
  char* writeData;
  ClientState state;

} TCP_client;


int TCP_client_init(TCP_client *_Client, uint16_t _Port, const char *_IP);
int TCP_client_connect(TCP_client *_Client);
int TCP_client_read(TCP_client *_Client, uint8_t *buf, int len);
int TCP_client_write(TCP_client *_Client, uint8_t *buf, int len);
void TCP_client_dispose(TCP_client *_Client);
*/
#endif
