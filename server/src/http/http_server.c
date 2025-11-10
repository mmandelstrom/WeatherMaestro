#include "../../include/http.h"
#include <stdio.h>
#include <stdlib.h>

/* -----------------Internal Functions----------------- */

void http_server_taskwork(void* _context, uint64_t _montime);
int http_server_on_accept(int _fd, void* _context);

/* ---------------------------------------------------- */

int http_server_init(HTTP_Server* _HTTPServer, http_server_on_connection _Callback, void* _ContextServer)
{
  if (!_HTTPServer || !_Callback) {
    return -1;
  }

  _HTTPServer->context = _ContextServer;
  _HTTPServer->on_connection = _Callback;
  _HTTPServer->state = HTTP_SERVER_INIT;
  _HTTPServer->task = NULL;

	if (tcp_server_init(&_HTTPServer->tcp_server, "58080", http_server_on_accept, _HTTPServer) <0 ) {
    perror("tcp_server_init");
    _HTTPServer->state = HTTP_SERVER_ERROR;
    _HTTPServer->task = scheduler_create_task(_HTTPServer, http_server_taskwork);
    return -2;
  }
  
  _HTTPServer->task = scheduler_create_task(_HTTPServer, http_server_taskwork);
  if (!_HTTPServer->task) {
    fprintf(stderr, "scheduler_create_task failed\n");
    tcp_server_dispose(&_HTTPServer->tcp_server);
    _HTTPServer->state = HTTP_SERVER_ERROR;
    return -3;
  }

  _HTTPServer->state = HTTP_SERVER_LISTENING;
	return 0;
}

int http_server_initiate_ptr(http_server_on_connection _callback, void* _context, HTTP_Server** _ServerPtr)
{
	if(_ServerPtr == NULL)
		return -1;

	HTTP_Server* _Server = (HTTP_Server*)malloc(sizeof(HTTP_Server));
	if(_Server == NULL)
		return -2;

	int result = http_server_init(_Server, _callback, _context);
	if(result != 0)
	{
		free(_Server);
		return result;
	}

	*(_ServerPtr) = _Server;

	return 0;
}

int http_server_connection_handover(int _fd, void* _Context)
{
  printf("handover\n");
  if (!_Context || _fd < 0) {
    return -1;
  }
	HTTP_Server* Server = (HTTP_Server*)_Context;

  /*TCP Server has accepted client*/
  Server->state = HTTP_SERVER_CONNECTING; 

	HTTP_Server_Connection* Connection = NULL;
	int result = http_server_connection_init_ptr(_fd, &Connection);
	if(result != 0)
	{
    Server->state = HTTP_SERVER_ERROR;
		printf("HTTP Server on TCP Accept: Failed to initiate connection\n");
		return -1;
	}

  printf("On accept _Server: %p\n", Server);

  result = Server->on_connection(Server, Connection);
  if (result != 0)
  {
    Server->state = HTTP_SERVER_ERROR;
    return -2;
  }

  /*IF httpserverconnection is initialized succesfully*/
  Server->state = HTTP_SERVER_CONNECTED;
	
	return _fd;
}

int http_server_on_accept(int _fd, void* _Context)
{
  printf("on accept fd: %i\n", _fd);
	HTTP_Server* Server = (HTTP_Server*)_Context;
  Server->client_fd = _fd;
  Server->state = HTTP_SERVER_CONNECTING;
	
	return 0;
}

void http_server_taskwork(void* _context, uint64_t _montime)
{
  if (!_context) {
    return;
  }

	HTTP_Server* server = (HTTP_Server*)_context;

  switch (server->state) {
    case HTTP_SERVER_INIT:
      break;

    case HTTP_SERVER_LISTENING: {
      int result = tcp_server_accept(&server->tcp_server);
      /* printf("clientfd: %i\n", server->client_fd); */
      if (result == -2) {
        server->state = HTTP_SERVER_ERROR;
      }
      else if (result == -1)
      {
        /* printf("Retrying\n"); */
      }
      else if (result == 0)
      {
        /* printf("Accepting\n"); */
      }
      break;
    }
    case HTTP_SERVER_CONNECTING: {
      http_server_connection_handover(server->client_fd, server);
      break;
    }
    case HTTP_SERVER_CONNECTED: 
    {
      server->state = HTTP_SERVER_LISTENING;
      /*Logic already handled in on_accept*/
      break;
    }
    case HTTP_SERVER_ERROR:
    /* Add http server error enum*/
    
      /*implement timeout logic here*/
    case HTTP_SERVER_DISPOSING: {
      tcp_server_dispose(&server->tcp_server);

      if (server->task) {
        scheduler_destroy_task(server->task);
        server->task = NULL;
      }

      http_server_dispose(server);
      break;
    }
    default:
      break;
  }
}


void http_server_dispose(HTTP_Server* _Server)
{
	tcp_server_dispose(&_Server->tcp_server);
	scheduler_destroy_task(_Server->task);
}

void http_server_dispose_ptr(HTTP_Server** _ServerPtr)
{
	if(_ServerPtr == NULL || *(_ServerPtr) == NULL)
		return;

	http_server_dispose(*(_ServerPtr));
	free(*(_ServerPtr));
	*(_ServerPtr) = NULL;
}
