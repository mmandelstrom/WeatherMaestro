#include "../../include/http.h"
#include <stdio.h>
#include <stdlib.h>

//-----------------Internal Functions-----------------

void http_server_taskwork(void* _Context, uint64_t _MonTime);
int http_server_on_accept(int _FD, void* _Context);

//----------------------------------------------------

int http_server_init(HTTP_Server* _Server, http_server_on_connection _callback)
{
	_Server->on_connection = _callback;

	tcp_server_init(&_Server->tcpServer, "58080", http_server_on_accept); 
	
	_Server->task = scheduler_create_task(_Server, http_server_taskwork);

	return 0;
}

int http_server_initiate_ptr(http_server_on_connection _callback, HTTP_Server** _ServerPtr)
{
	if(_ServerPtr == NULL)
		return -1;

	HTTP_Server* _Server = (HTTP_Server*)malloc(sizeof(HTTP_Server));
	if(_Server == NULL)
		return -2;

	int result = http_server_init(_Server, _callback);
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
	HTTP_Server* _Server = (HTTP_Server*)_Context;

	HTTP_Server_Connection* connection = NULL;
	int result = http_server_connection_initiate_ptr(_FD, &connection);
	if(result != 0)
	{
		printf("HTTP_Server_OnAccept: Failed to initiate connection\n");
		return -1;
	}

	_Server->on_connection(_Server, connection);
	
	return 0;
}

void http_server_taskwork(void* _Context, uint64_t _MonTime)
{
	//HTTP_Server* _Server = (HTTP_Server*)_Context;
	
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
