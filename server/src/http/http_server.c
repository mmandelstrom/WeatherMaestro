#include "../../include/http.h"
#include <stdio.h>
#include <stdlib.h>

//-----------------Internal Functions-----------------

void http_server_taskwork(void* _Context);
int http_server_on_accept(int _FD, void* _Context);

//----------------------------------------------------

int http_server_init(HTTP_Server* _HTTPServer, http_server_on_connection _Callback, void* _ContextServer)
{
  if (!_HTTPServer || !_Callback) {
    return -1;
  }

  _HTTPServer->context = _ContextServer;
  _HTTPServer->on_connection = _Callback;
  _HTTPServer->state = HTTP_SERVER_INIT;
  _HTTPServer->task = NULL;

	if (tcp_server_init(&_HTTPServer->tcpServer, "8080", http_server_on_accept, _HTTPServer) <0 ) {
    perror("tcp_server_init");
    _HTTPServer->state = HTTP_SERVER_ERROR;
    _HTTPServer->task = scheduler_create_task(_HTTPServer, http_server_taskwork);
    return -2;
  }
  
  _HTTPServer->task = scheduler_create_task(_HTTPServer, http_server_taskwork);
  if (!_HTTPServer->task) {
    fprintf(stderr, "scheduler_create_task failed\n");
    tcp_server_dispose(&_HTTPServer->tcpServer);
    _HTTPServer->state = HTTP_SERVER_ERROR;
    return -3;
  }

  _HTTPServer->state = HTTP_SERVER_LISTENING;
	return 0;
}

int http_server_initiate_ptr(http_server_on_connection _Callback, HTTP_Server** _ServerPtr)
{
	if(_ServerPtr == NULL)
		return -1;

	HTTP_Server* _Server = (HTTP_Server*)malloc(sizeof(HTTP_Server));
	if(_Server == NULL)
		return -2;

	int result = http_server_init(_Server, _Callback, NULL);
	if(result != 0)
	{
		free(_Server);
		return result;
	}

	*(_ServerPtr) = _Server;

	return 0;
}

int http_server_on_accept(int _FD, void* _Context)
{
  if (!_Context || _FD < 0) {
    return -1;
  }
	HTTP_Server* server = (HTTP_Server*)_Context;

  /*TCP Server has accepted client*/
  server->state = HTTP_SERVER_CONNECTING; 

	HTTP_Server_Connection* connection = NULL;
  /*Init real httpconnection*/

  /*IF httpserverconnection is initialized succesfully*/
  server->state = HTTP_SERVER_CONNECTED;
	server->on_connection(server, connection);


  /*After handover go back to listening*/
  server->state = HTTP_SERVER_LISTENING;
	
	return _FD;
}

void http_server_taskwork(void* _Context) {
  if (!_Context) {
    return;
  }

	HTTP_Server *server = (HTTP_Server*)_Context;

  switch (server->state) {
    case HTTP_SERVER_INIT:
      break;

    case HTTP_SERVER_LISTENING: {
      int result = tcp_server_accept(&server->tcpServer);
      if (result < 0) {
        server->state = HTTP_SERVER_ERROR;
      }
      break;
    }
    case HTTP_SERVER_CONNECTING:
    case HTTP_SERVER_CONNECTED:
      /*Logic already handled in on_accept*/
      break;
    case HTTP_SERVER_ERROR:

      /*implement timeout logic here*/
    case HTTP_SERVER_DISPOSING: {
      tcp_server_dispose(&server->tcpServer);

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
	tcp_server_dispose(&_Server->tcpServer);
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
