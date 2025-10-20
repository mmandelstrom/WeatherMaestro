#ifndef __TCP_H__
#define __TCP_H__
#include <stdint.h>
#include <stdbool.h>

typedef struct{
    int fd;
    uint16_t port;
    const char ip[16];
    bool valid;

}TCP_socket;


TCP_socket TCP_socket_init();

void TCP_socket_dispose(int* _Fd);

int TCP_client_con(int _Port, const char* _Socketaddress);

void TCP_client_dispose(int* _Fd);

#endif /* __TCP_H__ */