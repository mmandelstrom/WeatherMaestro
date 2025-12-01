#ifndef _REQUEST_HPP_
#define _REQUEST_HPP_

#include <string>

#include "tcp_client.hpp"


class Request {
    private:
        std::string city;
        std::string country;
        double lat;
        double lon;

    public:
        Request(std::string _City, std::string _Country); 
        int getWeather();
        int getCoords();



};

#endif