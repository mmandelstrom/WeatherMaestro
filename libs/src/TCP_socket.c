#include "../include/TCP_socket.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <string.h>

TCP_socket TCP_socket_init(){
    TCP_socket tcp_socket;
    tcp_socket.fd = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_socket.fd < 0){
        tcp_socket.valid = false;

        return tcp_socket;
    } 
    return tcp_socket;
}

int tcp_socket_set_reuseaddr(TCP_socket* _Tcp_socket){ /*If we want to disable reuse of address add int parameter in the future*/
    if (!_Tcp_socket){
        return -1;
}
int opt = 1;

return setsockopt(_Tcp_socket->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

int TCP_socket_bind(TCP_socket* _Tcp_socket){
    if (!_Tcp_socket){
        return -1;
    }
struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(_Tcp_socket->port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    return bind(_Tcp_socket->fd, (struct sockaddr*)&address, sizeof(address));

}

int TCP_socket_listen(TCP_socket* _Tcp_socket, int _Backlog){
    if (!_Tcp_socket){
        return -1;
    }
    if (_Backlog < 0){
        _Backlog = 0;
    }
    return listen(_Tcp_socket->fd, _Backlog);
}
int TCP_socket_accept(TCP_socket* _Tcp_socket){
    if (!_Tcp_socket){
        return -1;
    }
    struct sockaddr address;
    socklen_t addresslen = sizeof(address);
    return accept(_Tcp_socket->fd, &address, &addresslen);
}

int TCP_socket_client_setopt(TCP_socket* _Tcp_socket, struct sockaddr_in* _Address){
    if (!_Tcp_socket || !_Address){
        return -1;
    }
    _Address->sin_family = AF_INET;
    _Address->sin_port = htons(_Tcp_socket->port);

    return inet_pton(AF_INET, _Tcp_socket->ip, _Address->sin_addr);

}


void TCP_socket_dispose(int* _Fd);

int TCP_client_con(int _Port, const char* _Serveraddress);

void TCP_client_dispose(int* _Fd);