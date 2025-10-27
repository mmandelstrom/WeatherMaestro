#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__
#include <stdbool.h>
#include <stdint.h>

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

#endif
