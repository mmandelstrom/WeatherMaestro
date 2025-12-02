#include "request.hpp"
#include "json.hpp"

Request::Request(std::string _City, std::string _Country)
    : lat(59.3753f), lon(17.969f), http_response_code(-1)
    {
        this->city = _City;
        this->country = _Country;


    }

int Request::getWeather() {
    TCP_Client client;

    std::string request =
    "GET /api/v1/weather?latitude=" + std::to_string(this->lat) + "&longitude=" + std::to_string(this->lon) +" HTTP/1.1\r\n"
    "Host: stockholm2.onvo.se:81\r\n"
    "User-Agent: CPPCLIENT/0.1\r\n"
    "Accept: /\r\n"
    "Connection: close\r\n"
    "\r\n";

    client.setTransmitData(request);
    client.transmit();
    client.recieve();
    std::string response = client.getRecieveData();
    parseResponse(response);

    return 0;
}

int Request::parseResponse(std::string& _Response) {
    std::size_t firstline_end = _Response.find("\r\n");
    if (firstline_end == std::string::npos) {
        return -1;
    }
    this->http_response_firstline = _Response.substr(0, firstline_end);

    std::size_t body_start = _Response.find("{");
    this->http_response_body = _Response.substr(body_start);

    std::size_t code_start = this->http_response_firstline.find(" ");
    this->http_response_code = std::stoi(this->http_response_firstline.substr(code_start, 4));

    std::cout << "Code: " << this->http_response_code << std::endl;

    return 0;
}


// int Request::getCoords() {
//     TCP_Client client;

//     std::string request =
//     "GET /api/v1/geo/?city_name=" + this->city + " HTTP/1.1\r\n"
//     "Host: stockholm2.onvo.se:81\r\n"
//     "User-Agent: CPPCLIENT/0.1\r\n"
//     "Accept: /\r\n"
//     "Connection: close\r\n"
//     "\r\n";

//     client.setTransmitData(request);
//     client.transmit();
//     client.recieve();
//     std::string response = client.getRecieveData();

//     std::cout << response << std::endl;
// }

// [
//   {
//     "city": "Sundbyberg",
//     "country": "Sweden",
//     "latitude": 59.3753,
//     "longitude": 17.969
//   }
// ]