#include "../../libs/include/TCP_client.h"
#include <stdio.h>
#include <string.h>

int main() {
  const char *ip = "127.0.0.1";
  uint16_t port = 5555;
  TCP_client client;
  char *message = "Test from client\n";
  uint8_t buffer[1024];

  if (TCP_client_init(&client, port, ip) != 0) {
    printf("Failed to initialize client\n");
    return -1;
  }

  if (TCP_client_connect(&client) != 0) {
    printf("Failed to connect to %s:%d\n", ip, port);
  }

  
		while(1)
		{
			TCP_client_write(&client, (const uint8_t*)message, strlen(message));
			int bytesRead = TCP_client_read(&client, buffer, sizeof(buffer) - 1);
			if (bytesRead > 0)
			{
				buffer[bytesRead] = '\0';
				printf("%s", buffer);
			}
		}
  TCP_client_dispose(&client);

  return 0;
}
