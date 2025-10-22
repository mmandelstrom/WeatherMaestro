#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int fd;
  uint16_t port;
  char ip[16];
  bool isConnected;

} TCP_client;

int TCP_client_init(TCP_client *_Client, uint16_t _Port, const char *_IP);
int TCP_client_connect(TCP_client *_Client);
int TCP_client_read(TCP_client *_Client, uint8_t *buf, int len);
int TCP_client_write(TCP_client *_Client, uint8_t *buf, int len);
void TCP_client_dispose(TCP_client *_Client);

#endif
