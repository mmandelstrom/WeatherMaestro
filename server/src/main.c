#include "../../libs/include/tcp_client.h"
#include "../../libs/include/tcp_server.h"
#include <stdio.h>
#include <string.h>

#define BACKLOG 15

int main(void) {
  TCP_Server server;
  if (tcp_server_init(&server, "8080", BACKLOG) < 0) {
    perror("TCP_Server_init");
    return 1;
  }

  const char *geoString = "GET /geo?stockholm HTTP/1.1\r\n"
"Host: localhost:8080\r\n"
"User-Agent: curl/8.16.0\r\n"
"Accept: */*\r\n\r\n";

  const char *weatherString = "GET /weatherdata?latitude=59.3293&longitude=18.0686 HTTP/1.1\r\n"
"Host: localhost:8080\r\n"
"User-Agent: curl/8.16.0\r\n"
"Accept: */*\r\n\r\n";

  const char* geoResponse =
"HTTP/1.1 200 OK\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"\r\n"
"[\n"
"  {\n"
"    \"city\": \"Stockholm\",\n"
"    \"country\": \"Sweden\",\n"
"    \"latitude\": 59.3293,\n"
"    \"longitude\": 18.0686\n"
"  }\n"
"]\0";

  const char* weatherData = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json; charset=utf-8\r\n\r\n"
    "[\n"
"  {\n"
"    \"timestamp\": \"2025-10-31T07:99:59+01:00\",\n"
"    \"latitude\": 59.3753,\n"
"    \"longitude\": 17.969,\n"
"    \"interval\": 15,\n"
"    \"windspeed\": 2.4,\n"
"    \"winddirection\": \"nw\",\n"
"    \"temperature\": 13,\n"
"    \"is_day\": true,\n"
"    \"weathercode\": 1\n"
"  }\n"
"]\0";

const char* jsonError =
"HTTP/1.1 400 Bad Request\r\n"
"Content-Type: application/json; charset=utf-8\r\n\r\n"
"[\n"
"  {\n"
"    \"error\": {\n"
"      \"code\": 400,\n"
"      \"message\": \"bad request\"\n"
"    }\n"
"  }\n"
"]\0";

  while (1) {
    int cfd = tcp_server_accept(&server);
    if (cfd < 0) {
      /*No client currently*/
      continue;
    }

    TCP_Client client = {0};
    client.fd = cfd;

    int bytesRead = 0;
    int i;
    for (i = 0; i < 50; ++i) { /*500ms timeout*/
      bytesRead = tcp_client_read(&client);
      if (bytesRead != 0) break;
      /* sleep_ms(10); */
    }

    if (bytesRead > 0 && client.readData) {
      /*Print request in terminal*/
      printf("\n=== Incoming request (%d bytes) ===\n", bytesRead);
      printf("%.*s\n", bytesRead, client.readData);
      printf("===================================\n");
    } else if (bytesRead < 0) {
    
      if (client.readData) free(client.readData);
      close(client.fd);
      continue;
    }

    printf("readData: %s", client.readData);
    printf("Geostring: %s", geoString);

    if ((strcmp(geoString, client.readData)) == 0) {
      client.writeData = (char*)malloc(strlen(geoResponse) + 1);
        if (!client.writeData) {
          printf("Failed");
          return -1;
      }
      strcpy(client.writeData, geoResponse);
    } 

       else if (strcmp(weatherString, client.readData) == 0) {
        client.writeData = (char*)malloc(strlen(weatherData) + 1);
        if (!client.writeData) {
          printf("Failed");
          return -1;
      }
      strcpy(client.writeData, weatherData);
    } else {
       client.writeData = (char*)malloc(strlen(jsonError) + 1);
        if (!client.writeData) {
          printf("Failed");
          return -1;
      }
      strcpy(client.writeData, jsonError);
    }

    tcp_client_write(&client, strlen(client.writeData));

    if (client.readData)  free(client.readData);
    if (client.writeData) free(client.writeData);
    close(client.fd);
  }

  tcp_server_dispose(&server);
  return 0;
}

