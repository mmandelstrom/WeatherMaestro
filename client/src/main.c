#include "../../libs/include/TCP_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main() {
  const char *host = "127.0.0.1";
  const char *port = "5555";
  TCP_client client;
  char message[512];

  printf("Send a message to the sever: ");
  if (fgets(message, sizeof(message - 1), stdin) != NULL) {
    message[strcspn(message, "\n")] = '\0';
  }
  client.writeData = (char*)malloc(512 + 1);
  if (!client.writeData) return -1;

  strcpy(client.writeData, message);

  if (TCP_client_Initiate(&client, host, port) != 0) {
    printf("Failed to initialize client\n");
    return -1;
  }
 
  TCP_client_Dispose(&client);

  return 0;
}
