#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
// Deklarera funktionen från din server
void http_handle_request(const uint8_t *data, size_t len);


int main(int argc, char *argv[]) {
 
  uint8_t buf[65536];
  
  ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
  if (n <= 0) {
    return 0;
  }
// Här matar vi allt som en rå HTTP-request
  http_handle_request(buf, (size_t)n);
  return 0;
}
