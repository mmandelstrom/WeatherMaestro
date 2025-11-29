#include "tcp_client.hpp"

TCP_Client::TCP_Client(std::string _Host, std::string _Port) 
    : fd(-1), ready(false)
{
    std::cout << "TCP CLI CTOR" << std::endl; //DEBUG

    struct addrinfo connhints = {0}; 
    struct addrinfo* res = nullptr;

    connhints.ai_family = AF_UNSPEC;
    connhints.ai_socktype = SOCK_STREAM;
    connhints.ai_protocol = 6; // TCP

    int code = getaddrinfo(_Host.c_str(), _Port.c_str(), &connhints, &res);
    if(code != 0) {
        std::cout << "Error " << code << " while getting address info: " << gai_strerror(code) << std::endl;
        return;
    }

    int fd;
    while (res->ai_next != NULL)
    {
        fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (fd < 0) {continue;}

        if (int conn = connect(fd, res->ai_addr, res->ai_addrlen) == 0) {
            break;
        } else if (errno == EINPROGRESS) {
            break; //Nonblocking connection in progess (SOCKcess)
        }
        

        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    
    if (fd >= 3) {
    std::cout << "Socket successfully connected at fd " << fd << "!" << std::endl;
    ready = true;
    }
}

int TCP_Client::set_nonblocking(int _Fd){
    if (int flags = fcntl(_Fd, F_GETFL, 0) == -1) {return -1;} else 
    if (fcntl(_Fd, F_SETFL, flags | O_NONBLOCK) == -1) {return -1;} 
    return 0;
}

bool TCP_Client::is_ready() {return ready;}
