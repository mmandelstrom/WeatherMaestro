#include "request.hpp"
#include "json.hpp"

Request::Request(std::string _City, std::string _Country)
    : lat(59.3753f), lon(17.969f)
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

    std::cout << "Response: " << response << std::endl;
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