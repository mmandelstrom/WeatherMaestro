/* ******************************************************************* */
/* *************************** HTTP CLIENT *************************** */
/* ******************************************************************* */
#ifndef HTTPClient_h
#define HTTPClient_h
#include "../../server/include/scheduler.h"
#include "../../server/include/error.h"
#include "../../libs/include/HTTPStatusCodes.h"
#include "../../server/include/tcp/tcp_client.h"
#include "../../server/include/http/http_parser.h"
#include <stdint.h>
#include <stdbool.h>


typedef enum
{
  HTTP_CLIENT_INITIALIZING,
  HTTP_CLIENT_CONNECTING,
  HTTP_CLIENT_BUILD_REQUEST,
  HTTP_CLIENT_SEND_REQUEST,
  HTTP_CLIENT_RECEIVE_RESPONSE,
  HTTP_CLIENT_DISPOSING,
  HTTP_CLIENT_ERROR,

} HTTPClientState;

typedef const char (*http_client_on_success)();

typedef struct
{
  http_client_on_success    on_success;
	uint8_t*                  request_buffer;
	uint8_t*                  request_bufferPtr;
  int                       request_length;
  uint8_t*                  response_buffer;
  uint8_t*                  response_bufferPtr;
	Scheduler_Task*           task;
  TCP_Client*               tcp_client;
  HTTPClientState           state;
  const char*               URL;
  HTTPMethod                method;
  bool                      tls;
  int                       bytes_sent;
  int                       bytes_received;
  int                       retries;
  uint64_t                  next_retry_at;
} HTTP_Client;


int http_client_initiate(HTTP_Client* _Client,  http_client_on_success _on_success, const char* _URL, HTTPMethod _method);
void http_client_dispose(HTTP_Client* _Client);

#endif //HTTPClient_h
