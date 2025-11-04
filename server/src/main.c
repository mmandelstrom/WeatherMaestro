#include "../include/tcp.h"
#include <stdio.h>
#include <string.h>

#define BACKLOG 15

int main(void) {
  TCP_Server server;
  if (tcp_server_init(&server, "8080", BACKLOG) < 0) {
    perror("TCP_Server_init");
    return 1;
  }


  const char* optionsResponse =
"HTTP/1.1 204 No Content\r\n"
"Access-Control-Allow-Origin: *\r\n"
"Access-Control-Allow-Methods: GET, OPTIONS\r\n"
"Access-Control-Allow-Headers: Content-Type\r\n"
"Access-Control-Max-Age: 86400\r\n"
"Content-Length: 0\r\n"
"Connection: close\r\n"
"\r\n";

  const char* geoResponse =
"HTTP/1.1 200 OK\r\n"
"Access-Control-Allow-Origin: *\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Connection: close\r\n"
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
"Access-Control-Allow-Origin: *\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Connection: close\r\n"
"\r\n"
"[\n"
"  {\n"
"    \"timestamp\": \"2025-10-31T07:59:59+01:00\",\n"
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
"Access-Control-Allow-Origin: *\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Connection: close\r\n"
"\r\n"
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

    for (i = 0; i < 200; ++i) {        /* ≈2s total */
      bytesRead = tcp_client_read(&client);
      if (bytesRead != 0) break;       /* >0 done, <0: error */
        usleep(10000);
      }
   
    if (bytesRead > 0 && client.readData) {
      if (strncmp(client.readData, "OPTIONS ", 8) == 0) {
        client.writeData = strdup(optionsResponse);
      }
      else if (strncmp(client.readData, "GET /api/v1/geo?city_name=", sizeof("GET /api/v1/geo?city_name=") - 1) == 0) {
        client.writeData = strdup(geoResponse);
      }
      else if (strncmp(client.readData, "GET /api/v1/weatherdata?", sizeof("GET /api/v1/weatherdata?") - 1) == 0) {
        client.writeData = strdup(weatherData);
      }
      else {
        client.writeData = strdup(jsonError);
      }
      if (!client.writeData) { perror("malloc"); close(client.fd); continue; }
      tcp_client_write(&client, strlen(client.writeData));
    }


    if (client.readData)  free(client.readData);
    if (client.writeData) free(client.writeData);
    close(client.fd);
  }

  tcp_server_dispose(&server);
  return 0;
}
