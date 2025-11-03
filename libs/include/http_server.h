#ifndef __HTTP_SERVER_H_
#define __HTTP_SERVER_H_

/* === HTTP SERVER CONNECTION === */

#include "smw.h"
#include "http_status_codes.h"
#include "TCP_server.h"
#include "TCP_client.h"
#include "../../utils/include/utils.h"

/* #include "../utils.h" */

typedef int (*HTTPServerConnection_OnRequest)(void* _Context);

typedef enum
{
  HTTP_SERVER_INITING,
  HTTP_SERVER_READING_FIRSTLINE,
  HTTP_SERVER_READING_HEADERS,
  HTTP_SERVER_READING_BODY,
  HTTP_SERVER_RESPONDING,
  HTTP_SERVER_CALLBACKING,
  HTTP_SERVER_DONE

} Http_Server_Connection_State;

typedef struct
{
	TCP_client tcpClient;

	void* context;
	HTTPServerConnection_OnRequest onRequest;

  enum HttpStatus_Code status_code;
  char* host; // Request URL 
	char* method; // HTTP method
  char* response; // Request response

  Http_Server_Connection_State state;

	smw_task* task;

} HTTPServerConnection;


int HTTPServerConnection_Initiate(HTTPServerConnection* _Connection, int _FD);
int HTTPServerConnection_InitiatePtr(int _FD, HTTPServerConnection** _ConnectionPtr);

void HTTPServerConnection_SetCallback(HTTPServerConnection* _Connection, void* _Context, HTTPServerConnection_OnRequest _OnRequest);

void HTTPServerConnection_Dispose(HTTPServerConnection* _Connection);
void HTTPServerConnection_DisposePtr(HTTPServerConnection** _ConnectionPtr);

/* === HTTP SERVER === */

typedef int (*HTTPServer_OnConnection)(void* _Context, HTTPServerConnection* _Connection);

typedef struct
{
	HTTPServer_OnConnection onConnection;

	TCP_server tcpServer;
	smw_task* task;

} HTTPServer;


int HTTPServer_Initiate(HTTPServer* _Server, HTTPServer_OnConnection _OnConnection);
int HTTPServer_InitiatePtr(HTTPServer_OnConnection _OnConnection, HTTPServer** _ServerPtr);


void HTTPServer_Dispose(HTTPServer* _Server);
void HTTPServer_DisposePtr(HTTPServer** _ServerPtr);


#endif //__HTTPServer_h_
