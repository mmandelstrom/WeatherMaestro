#ifndef __HTTP_SERVER_H_
#define __HTTP_SERVER_H_

#include "tcp.h"
#include "scheduler.h"


/* ******************************************************************* */
/* ************************ HTTP CONNECTION ************************** */
/* ******************************************************************* */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../../libs/include/HTTPStatusCodes.h"
#include "../../utils/include/utils.h"


typedef int (*http_server_connection_on_request)(void* _Context);

typedef enum
{
  HTTP_SERVER_INITING,
  HTTP_SERVER_READING_FIRSTLINE,
  HTTP_SERVER_READING_HEADERS,
  HTTP_SERVER_READING_BODY,
  HTTP_SERVER_RESPONDING,
  HTTP_SERVER_CALLBACKING,
  HTTP_SERVER_DONE

} HTTPServerConnectionState;

typedef struct
{
	TCP_Client tcpClient;

	void* context;
	http_server_connection_on_request onRequest;

  enum HttpStatus_Code status_code;
  char* host; // Request URL 
	char* method; // HTTP method
  char* response; // Request response

  HTTPServerConnectionState state;

	Scheduler_Task* task;

} HTTP_Server_Connection;


int http_server_connection_initiate(HTTP_Server_Connection* _Connection, int _FD);
int http_server_connection_initiate_ptr(int _FD, HTTP_Server_Connection** _ConnectionPtr);

void http_server_connection_setcallback(HTTP_Server_Connection* _Connection, void* _Context, http_server_connection_on_request _OnRequest);

void http_server_connection_dispose(HTTP_Server_Connection* _Connection);
void http_server_connection_dispose_ptr(HTTP_Server_Connection** _ConnectionPtr);


/* ******************************************************************* */
/* ************************** HTTP SERVER **************************** */
/* **** **************************************************************** */
/* This is the main HTTP building block
 * It spawns the TCP Server
 * Also a scheduler task for every connection made */

typedef enum {
  HTTP_SERVER_INIT,
  HTTP_SERVER_LISTENING,
  HTTP_SERVER_CONNECTING,
  HTTP_SERVER_CONNECTED,
  HTTP_SERVER_ERROR,
  HTTP_SERVER_DISPOSING
} HTTP_ServerState;

typedef int (*http_server_on_connection)(void* _Context, HTTP_Server_Connection* _Connection);

typedef struct
{
  void* context;
	TCP_Server tcpServer;
	Scheduler_Task* task;
  HTTP_ServerState state;
  http_server_on_connection on_connection;

} HTTP_Server;


int http_server_init(HTTP_Server* _Server, http_server_on_connection _OnConnection, void* _Context);
int http_server_init_ptr(http_server_on_connection _OnConnection, HTTP_Server** _ServerPtr);
int http_server_on_accept(int _FD, void* _Context);
void http_server_dispose(HTTP_Server* _Server);
void http_server_dispose_ptr(HTTP_Server** _ServerPtr);


#endif //__HTTP_Server_h_
