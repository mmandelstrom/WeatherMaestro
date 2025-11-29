
#include "tcpclient.hpp"

int main() {
  TCP_Client cli("stockholm2.onvo.se", "81");

  if (cli.is_ready()) {
    std::cout << "Ready!" << std::endl;
  }

  return 0; 
}
