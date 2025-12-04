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
  HTTP_CLIENT_READ_FIRSTLINE,
  HTTP_CLIENT_READ_HEADERS,
  HTTP_CLIENT_READ_BODY,
  HTTP_CLIENT_RETURNING,
  HTTP_CLIENT_DISPOSING,
  HTTP_CLIENT_ERROR,

} HTTPClientState;

typedef const char (*http_client_on_success)();

typedef struct
{
  http_client_on_success    on_success;
	uint8_t*                  request_buffer;
  int                       request_length;
  uint8_t*                  response_buffer;
	Scheduler_Task*           task;
  TCP_Client*               tcp_client;
  HTTPClientState           state;
  const char*               URL;
  HTTPMethod                method;
  bool                      tls;
  size_t                    bytes_sent;
  int                       bytes_received;
  int                       retries;
  uint64_t                  next_retry_at;
  Linked_List*              params;
  Linked_List*              headers;
  int                       params_count;
  int                       content_length;
  HTTP_Request*             req;
  HTTP_Response*            resp;
} HTTP_Client;


int http_client_initiate(HTTP_Client* _Client, const char* _URL, HTTPMethod _method);
void http_client_dispose(HTTP_Client* _Client);

#endif //HTTPClient_h
