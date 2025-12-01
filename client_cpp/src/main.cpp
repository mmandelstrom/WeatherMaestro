
#include "tcp_client.hpp"

int main() {
  TCP_Client cli("stockholm2.onvo.se", "81");

  std::string request =
    "GET /api/v1/weather?latitude=52.125&longitude=53.123445463624674272457 HTTP/1.1\r\n"
    "Host: stockholm2.onvo.se:81\r\n"
    "User-Agent: MyClient/1.0\r\n"
    "Accept: /\r\n"
    "Connection: close\r\n"
    "\r\n";

  cli.setTransmitData(request);
  cli.transmit();

  cli.recieve();


  return 0; 
}
