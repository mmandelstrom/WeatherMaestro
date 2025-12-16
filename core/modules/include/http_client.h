/* ******************************************************************* */
/* *************************** HTTP CLIENT *************************** */
/* ******************************************************************* */
#ifndef HTTPClient_h
#define HTTPClient_h

#include "HTTPStatusCodes.h"
#include "scheduler.h"
#include "error.h"
#include "tcp_client.h"
#include "http_parser.h"

#include <stdint.h>
#include <stdbool.h>


typedef enum
{
  HTTP_CLIENT_INITIALIZING,
  HTTP_CLIENT_CONNECTING,
  HTTP_CLIENT_BUILDING_REQUEST,
  HTTP_CLIENT_SENDING_REQUEST,
  HTTP_CLIENT_READING_FIRSTLINE,
  HTTP_CLIENT_READING_HEADERS,
  HTTP_CLIENT_READING_BODY,
  HTTP_CLIENT_RETURNING,
  HTTP_CLIENT_DISPOSING,
  HTTP_CLIENT_ERROR,

} HTTPClientState;

typedef void (*http_client_on_success)(void* _context, char** _response);

typedef struct {
  char        scheme[6]; // http/https
  char        host[128];
  char        port[6];
  char        path[512];
} URL_Parts;


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
  URL_Parts                 url_parts;

  void*                     context;
  char**                    response_out;
} HTTP_Client;


int http_client_initiate(HTTP_Client* _Client, const char* _URL, HTTPMethod _method, http_client_on_success _on_success, void* _context, char** _response_out);
void http_client_dispose(HTTP_Client* _Client);

#endif //HTTPClient_h
