#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../include/http_server.h"
#include "../include/http_status_codes.h"

//-----------------Internal Functions-----------------

void HTTPServerConnection_TaskWork(void* _Context, uint64_t _MonTime);

//----------------------------------------------------

#define RESPONSE_TEMPLATE "HTTP/1.1 %i %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s"

int HTTPServerConnection_Initiate(HTTPServerConnection* _Connection, int _fd)
{
	TCP_client_init(&_Connection->tcpClient, _fd);
	
	_Connection->task = smw_create_task(_Connection, HTTPServerConnection_TaskWork);
  _Connection->state = 0;
  _Connection->status_code = HttpStatus_Continue;

	return 0;
}

int HTTPServerConnection_InitiatePtr(int _fd, HTTPServerConnection** _ConnectionPtr)
{
	if(_ConnectionPtr == NULL)
		return -1;

	HTTPServerConnection* _Connection = (HTTPServerConnection*)malloc(sizeof(HTTPServerConnection));
	if(_Connection == NULL)
		return -2;

	int result = HTTPServerConnection_Initiate(_Connection, _fd);
	if(result != 0)
	{
		free(_Connection);
		return result;
	}

	*(_ConnectionPtr) = _Connection;

	return 0;
}

void HTTPServerConnection_SetCallback(HTTPServerConnection* _Connection, void* _Context, HTTPServerConnection_OnRequest _OnRequest)
{
  _Connection->context = _Context;
  _Connection->onRequest = _OnRequest;
}

// --- TASKWORK STATE FUNCTIONS ---
Http_Server_Connection_State http_server_work_init(HTTPServerConnection* _Connection)
{
  return HTTP_SERVER_READING_FIRSTLINE;
}
Http_Server_Connection_State http_server_work_read_firstline(HTTPServerConnection* _Connection)
{
  size_t used_space = 0;
  size_t capacity = 4;
  char* request = (char*)malloc(capacity + 1); // +1 for null terminator
  if (request == NULL)
  {
    perror("malloc");
    return -1;
  }

  uint64_t now = SystemMonotonicMS();
  uint64_t timeout = now + 5000;
  while (now < timeout)
  {
    /* Expand buffer capacity if needed */
    if (used_space >= capacity)
    {
      size_t new_capacity = capacity * 2;
      char* temp_buf = (char*)realloc(request, new_capacity + 1);
      if (temp_buf == NULL)
      {
        free(request);
        perror("realloc");
        return HTTP_SERVER_DONE;
      }
      capacity = new_capacity;
      request = temp_buf;
    }

    int bytes_recvd = TCP_client_read(&_Connection->tcpClient, (uint8_t*)request + used_space, capacity - used_space);
    if (bytes_recvd > 0)
    {
      used_space += bytes_recvd;
      request[used_space] = '\0';

      char* line_end = strstr(request, "\r\n");
      if (line_end != NULL)
      {
        /* size_t line_length = line_end - request;
        printf("First line: %.*s\n", (int)line_length, request); */

        _Connection->method = strdup(strtok(request, " "));

        free(request);
        return HTTP_SERVER_READING_HEADERS;
      }
      // else continue reading more data
    }
    else if (bytes_recvd == 0)
    {
      /* Connection closed before first line read */
      free(request);
      perror("connection_closed");
      return HTTP_SERVER_DONE;
    }
    else
    {
      if (errno != EAGAIN || errno != EWOULDBLOCK)
      {
        free(request);
        perror("TCPClient_Read");
        return HTTP_SERVER_DONE;
      }
    }
  }
  perror("timeout");
  return HTTP_SERVER_DONE;
}
Http_Server_Connection_State http_server_work_read_headers(HTTPServerConnection* _Connection)
{
  size_t used_space = 0;
  size_t capacity = 4;
  char* request = (char*)malloc(capacity + 1); // +1 for null terminator
  if (request == NULL)
  {
    perror("malloc");
    return -1;
  }

  uint64_t now = SystemMonotonicMS();
  uint64_t timeout = now + 5000;
  while (now < timeout)
  {
    /* Expand buffer capacity if needed */
    if (used_space >= capacity)
    {
      size_t new_capacity = capacity * 2;
      char* temp_buf = (char*)realloc(request, new_capacity + 1);
      if (temp_buf == NULL)
      {
        free(request);
        perror("realloc");
        return HTTP_SERVER_DONE;
      }
      capacity = new_capacity;
      request = temp_buf;
    }

    int bytes_recvd = TCPClient_Read(&_Connection->tcpClient, (uint8_t*)request + used_space, capacity - used_space);
    if (bytes_recvd > 0)
    {
      used_space += bytes_recvd;
      request[used_space] = '\0';

      char* headers_end = strstr(request, "\r\n\r\n"); // Empty line separates headers from body
      if (headers_end != NULL)
      {
        /* size_t headers_length = headers_end - request;
        printf("Headers: %.*s\n", (int)headers_length, request); */
        printf("Request headers: %s\n", request);

        /* Loop through each line in request headers 
         * Also until request is over altogether 
         * if we want to read body then nullterm condition should be removed*/
        char* header = strtok(request, "\r\n");
        while (header != NULL) {
          char* colon = strchr(header, ':');
          if (!colon) 
          {
            header = strtok(NULL, "\r\n"); 
            continue;
          }
          
          *colon = '\0';
          char* key = header;
          char* value = colon + 1;

          printf("Key: %s, Value: %s\n", key, value);

          /* Trim leading whitespace from value */
          while (*value == ' ') value++;

          /* Extract host header */
          if (strcasecmp(key, "Host") == 0)
            _Connection->host = strdup(value);

          header = strtok(NULL, "\r\n");
        }

        free(request);
        return HTTP_SERVER_READING_BODY;
      }
      // else continue reading more data
    }
    else if (bytes_recvd == 0)
    {
      /* Connection closed before first line read */
      free(request);
      perror("connection_closed");
      return HTTP_SERVER_DONE;
    }
    else
    {
      if (errno != EAGAIN || errno != EWOULDBLOCK)
      {
        free(request);
        perror("TCPClient_Read");
        return HTTP_SERVER_DONE;
      }
    }
  }
  perror("timeout");
  return HTTP_SERVER_DONE;
}
Http_Server_Connection_State http_server_work_read_body(HTTPServerConnection* _Connection)
{
  // TBA
  return HTTP_SERVER_RESPONDING;
}
Http_Server_Connection_State http_server_work_respond(HTTPServerConnection* _Connection)
{
  _Connection->status_code = HttpStatus_OK;

  /* If Method is GET we respond with URL and method */
  if (_Connection->method == NULL || _Connection->host == NULL)
  {
    _Connection->status_code = HttpStatus_Invalid;
    perror("bad_http");
    return HTTP_SERVER_DONE;
  }

  if (strcmp(_Connection->method, "GET") == 0)
  {
    const char* reason_phrase = HttpStatus_reasonPhrase(_Connection->status_code);

    int response_length = strlen(RESPONSE_TEMPLATE) +
        strlen(reason_phrase) + 
        strlen(_Connection->method) +
        strlen(_Connection->host) + 
        5; // Max status code length and nullterm

    /* We just want to send back method and host used in reqest for now */
    char* body = stringcat(_Connection->method, _Connection->host);

    char* response = (char*)malloc(response_length);
    snprintf(response, 
        response_length, 
        RESPONSE_TEMPLATE,
        _Connection->status_code,
        reason_phrase,
        (int)strlen(body),
        body);

    free(body);

    _Connection->response = response;

    printf("RESPONSE: %s\n", _Connection->response);

    int bytes_sent = TCPClient_Write(&_Connection->tcpClient, 
        (const uint8_t*)_Connection->response, 
        strlen(_Connection->response));

    if (bytes_sent < 0) {
      perror("TCPClient_Write");
      return HTTP_SERVER_DONE; // error
    }
  }

  return HTTP_SERVER_CALLBACKING;
}
Http_Server_Connection_State http_server_work_callback(HTTPServerConnection* _Connection)
{
  if(_Connection->onRequest != NULL)
  {
    printf("------------");
    _Connection->onRequest("THERE HAS BEEN A GET REQUEST\n");
    printf("Method: %s\n", _Connection->method);
    printf("Host: %s\n", _Connection->host);
  }

  return HTTP_SERVER_DONE;
}
// -----------------------

void HTTPServerConnection_TaskWork(void* _Context, uint64_t _MonTime)
{
	HTTPServerConnection* _Connection = (HTTPServerConnection*)_Context;

  switch (_Connection->state)
  {
    case HTTP_SERVER_INITING:
    {
      _Connection->state = http_server_work_init(_Connection);
    } break;

    case HTTP_SERVER_READING_FIRSTLINE:
    {
      _Connection->state = http_server_work_read_firstline(_Connection);
    } break;

    case HTTP_SERVER_READING_HEADERS:
    {
      _Connection->state = http_server_work_read_headers(_Connection);
    } break;

    case HTTP_SERVER_READING_BODY:
    {
      _Connection->state = http_server_work_read_body(_Connection);
    } break;

    case HTTP_SERVER_RESPONDING:
    {
      _Connection->state = http_server_work_respond(_Connection);
    } break;

    case HTTP_SERVER_CALLBACKING:
    {
      _Connection->state = http_server_work_callback(_Connection);
    } break;

    case HTTP_SERVER_DONE:
    {
      HTTPServerConnection_Dispose(_Connection);
    } break;
  }
}

void HTTPServerConnection_Dispose(HTTPServerConnection* _Connection)
{
	TCPClient_Dispose(&_Connection->tcpClient);
	smw_destroyTask(_Connection->task);
}

void HTTPServerConnection_DisposePtr(HTTPServerConnection** _ConnectionPtr)
{
	if(_ConnectionPtr == NULL || *(_ConnectionPtr) == NULL)
		return;

	HTTPServerConnection_Dispose(*(_ConnectionPtr));

  /* Free malloced member strings */
  if ((*_ConnectionPtr)->method != NULL)
    free((*_ConnectionPtr)->method);
  if ((*_ConnectionPtr)->host != NULL)
    free((*_ConnectionPtr)->host);
  if ((*_ConnectionPtr)->response != NULL)
    free((*_ConnectionPtr)->response);

	free(*(_ConnectionPtr));
	*(_ConnectionPtr) = NULL;
}
