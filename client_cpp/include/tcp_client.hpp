#ifndef _TCP_CLIENT_HPP_
#define _TCP_CLIENT_HPP_

#define _POSIX_C_SOURCE 200112L

#include <string>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

//
//
//
//
//

class TCP_Client {
    private:
        int fd;
        std::string transmit_data;
        std::string receive_data;
        bool ready;

        int set_nonblocking(int _Fd);

    public:
        TCP_Client(std::string _Host, std::string _Port);
        ~TCP_Client();

        bool is_ready();
        int read();
        int write();
        int getFileDescriptor();

};

#endif