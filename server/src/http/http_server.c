#include "../../include/http.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* -----------------Internal Functions----------------- */
void http_server_taskwork(void* _context, uint64_t _montime);
int http_server_on_accept(int _fd, void* _context);
int http_retry_tcp_init(void *_args);
HTTPServerState http_server_retry_work(HTTP_Server* _Server);
HTTPServerState http_server_error_work(HTTP_Server* _Server);
/* ---------------------------------------------------- */

int http_server_init(HTTP_Server* _HTTPServer, http_server_on_connection _Callback, void* _ContextServer)
{
  if (!_HTTPServer || !_Callback) {
    errno = EINVAL; /*Invalid argument*/
    return -1;
  }

  _HTTPServer->context = _ContextServer;
  _HTTPServer->on_connection = _Callback;
  _HTTPServer->state = HTTP_SERVER_INIT;
  _HTTPServer->error_state = HTTP_SERVER_ERROR_NONE;
  _HTTPServer->task = NULL;
  _HTTPServer->client_fd = -1;
  _HTTPServer->error_retries = 0;
  _HTTPServer->next_retry_at = 0;
  _HTTPServer->retry_args = NULL;
  _HTTPServer->retry_function = NULL;

	if (tcp_server_init(&_HTTPServer->tcp_server, "58080", http_server_on_accept, _HTTPServer) <0 ) {
    perror("tcp_server_init");
    _HTTPServer->state = HTTP_SERVER_ERROR;
    _HTTPServer->error_state = HTTP_SERVER_ERROR_TCP_INIT_FAILED;

    HTTP_Tcp_Init_Args* args = (HTTP_Tcp_Init_Args*)malloc(sizeof(HTTP_Tcp_Init_Args));
    if (!args) {
      errno = ENOMEM;
      return -1;
    }
   
    /*Pack the arguments needed for retrying tcp_init*/
    args->tcp_server = &_HTTPServer->tcp_server;
    args->port = "58080";
    args->on_accept = http_server_on_accept;
    args->context = _HTTPServer;

    _HTTPServer->error_retries = 0;
    _HTTPServer->retry_function = http_retry_tcp_init;
    _HTTPServer->retry_args = args;
    _HTTPServer->next_retry_at = SystemMonotonicMS() + 30000; /*30s timeout between retries*/

    _HTTPServer->task = scheduler_create_task(_HTTPServer, http_server_taskwork);

    return -1;
  }
 
  /*This wont matter as if there is no task nothing will execute error code*/
  _HTTPServer->task = scheduler_create_task(_HTTPServer, http_server_taskwork);
  if (!_HTTPServer->task) {
    fprintf(stderr, "scheduler_create_task failed\n");
    tcp_server_dispose(&_HTTPServer->tcp_server);
    _HTTPServer->state = HTTP_SERVER_ERROR;
    errno = EAGAIN;
    return -1;
  }

  _HTTPServer->state = HTTP_SERVER_IDLE;
	return 0;
}

int http_server_initiate_ptr(http_server_on_connection _callback, void* _context, HTTP_Server** _ServerPtr)
{
	if(_ServerPtr == NULL) {
    errno = EINVAL;
    return -1;
  }

  HTTP_Server* _Server = (HTTP_Server*)malloc(sizeof(HTTP_Server));
	if(_Server == NULL) {
    errno = ENOMEM;
    return -1;
  }

	int result = http_server_init(_Server, _callback, _context);
	if(result != 0)
	{
    perror("http_server_init");
		free(_Server);
		return result;
	}

	*(_ServerPtr) = _Server;

	return 0;
}

HTTPServerState http_server_connection_handover(int _fd, void* _Context)
{
  if (!_Context || _fd < 0) {
    errno = EINVAL;
    return -1;
  }

	HTTP_Server* Server = (HTTP_Server*)_Context;

  /*TCP Server has accepted client*/

	HTTP_Server_Connection* Connection = NULL;
	int result = http_server_connection_init_ptr(_fd, &Connection);
	if(result != 0)
	{
    perror("http_server_connection_init_ptr");
    close(_fd);
    Server->client_fd = -1;
    return HTTP_SERVER_IDLE;
  }


  result = Server->on_connection(Server, Connection);
  if (result != 0)
  {
    perror("Server->on_connection");
    close(_fd);

    Server->client_fd = -1;
    return HTTP_SERVER_IDLE;
  }

  /*IF httpserverconnection is initialized succesfully*/
  return HTTP_SERVER_CONNECTED;
}

int http_server_on_accept(int _fd, void* _Context)
{
  if (!_Context || _fd < 0) {
    errno = EINVAL;
    return -1;
  }

  printf("on accept fd: %i\n", _fd);
	HTTP_Server* Server = (HTTP_Server*)_Context;
  Server->client_fd = _fd;
  Server->state = HTTP_SERVER_CONNECTING;

	return 0;
}

void http_server_taskwork(void* _context, uint64_t _montime)
{
  if (!_context) {
    errno = EINVAL;
    return;
  }

	HTTP_Server* server = (HTTP_Server*)_context;

  HTTPServerState next_state = server->state;

  switch (server->state) {
    case HTTP_SERVER_INIT:
      break;

    case HTTP_SERVER_IDLE: {
      break;        
    }
      
    case HTTP_SERVER_CONNECTING: {
      printf("HTTP_SERVER_CONNECTING\n");
      next_state = http_server_connection_handover(server->client_fd, server);
      break;
    }
    case HTTP_SERVER_CONNECTED: 
    {
      printf("HTTP_SERVER_CONNECTED\n");
      next_state = HTTP_SERVER_IDLE;
      break;
    }
    case HTTP_SERVER_ERROR:
      printf("HTTP_SERVER_ERROR\n");
      next_state = http_server_error_work(server);
      break;

    case HTTP_SERVER_DISPOSING: {
      printf("HTTP_SERVER_DISPOSING\n");
      http_server_dispose(server);
      return;
    }

    default:
      break;
  }

  server->state = next_state;

}

int http_retry_tcp_init(void *_args) {
    HTTP_Tcp_Init_Args *args = (HTTP_Tcp_Init_Args*)_args;
    return tcp_server_init(args->tcp_server, args->port, args->on_accept, args->context);
}

HTTPServerState http_server_error_work(HTTP_Server* _Server) {

  switch(_Server->error_state) {

    case HTTP_SERVER_ERROR_NONE:
      return HTTP_SERVER_IDLE;

    case HTTP_SERVER_ERROR_INVALID_ARGUMENT:
    /*Errors not solved by retry*/
      return HTTP_SERVER_DISPOSING;

    case HTTP_SERVER_ERROR_ACCEPT_FAILED:
      return http_server_retry_work(_Server);

    case HTTP_SERVER_ERROR_TCP_INIT_FAILED:
      return http_server_retry_work(_Server);

    default:
      return HTTP_SERVER_DISPOSING;
  }

  return HTTP_SERVER_DISPOSING;

}

HTTPServerState http_server_retry_work(HTTP_Server* _Server) {
  if (!_Server) {
    errno = EINVAL;
    return HTTP_SERVER_DISPOSING;
  }

  uint64_t now = SystemMonotonicMS();
  
  if (now < _Server->next_retry_at)
    return HTTP_SERVER_ERROR;
  
  if (!_Server->retry_function) {
    return HTTP_SERVER_DISPOSING;
  }

  int result = _Server->retry_function(_Server->retry_args);

  if (result == 0) {
    free(_Server->retry_args);
    
    _Server->retry_args = NULL;
    _Server->retry_function = NULL;
    _Server->error_state = HTTP_SERVER_ERROR_NONE;
    _Server->error_retries = 0;
    return HTTP_SERVER_IDLE;

  } else {
    _Server->error_retries++;

    if (_Server->error_retries > 3) {

      free(_Server->retry_args);
      _Server->retry_args = NULL;
      _Server->retry_function = NULL;
      return HTTP_SERVER_DISPOSING;

    } else {
      _Server->next_retry_at = now + 30000;
      return HTTP_SERVER_ERROR;
    }
  }
}

void http_server_dispose(HTTP_Server* _Server)
{
  if (!_Server) return;

	tcp_server_dispose(&_Server->tcp_server);
  if (_Server->task) {
    scheduler_destroy_task(_Server->task);
    _Server->task = NULL;
  }
}

void http_server_dispose_ptr(HTTP_Server** _ServerPtr)
{
	if(_ServerPtr == NULL || *(_ServerPtr) == NULL)
		return;

	http_server_dispose(*(_ServerPtr));
	free(*(_ServerPtr));
	*(_ServerPtr) = NULL;
}
