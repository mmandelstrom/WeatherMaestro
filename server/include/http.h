#ifndef __HTTP_SERVER_H_
#define __HTTP_SERVER_H_


/* ******************************************************************* */
/* ************************** HTTP PARSING *************************** */
/* ******************************************************************* */

#include <stdint.h>

#include "tcp.h"

#include "../../libs/include/HTTPStatusCodes.h"
#include "../../libs/include/yuarel.h"
#include "../../utils/include/utils.h"
#include "../../utils/include/linked_list.h"

#define TCP_MESSAGE_BUFFER_MAX_SIZE 128 // Size of initial tcp_read buffer without reallocating more mem
#define HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN 1024 // Maximum length of http request's first line

typedef enum
{
  HTTP_OPTIONS,
  HTTP_GET,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_DOWNLOAD,

  HTTP_INVALID

} HTTPMethod;

typedef struct
{
  enum HttpStatus_Code status_code;

  const char*          head;
  const char*          headers;
  const char*          body;  

} HTTP_Response;

typedef struct
{
  HTTPMethod      method;

  char*           method_str;
  char*           path; 
  char*           query;
  char*           version;

  Linked_List*    headers;

  char*           body;

} HTTP_Request;


char* http_build_full_response(int _status_code, const char* _reason_phrase, const char* _method, const char* _path, Linked_List* _Headers);

HTTPMethod http_method_string_to_enum(const char* _method_str);
const char* http_method_enum_to_string(HTTPMethod _method);


/* ******************************************************************* */
/* ************************ HTTP CONNECTION ************************** */
/* ******************************************************************* */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "scheduler.h"


/* The usecase of the function pointer is to let the connection instance point back to a server nstance's function without knowing exactly what it is or needs */
typedef int (*http_server_connection_on_request)(void* _context);

typedef enum
{
  HTTP_SERVER_CONNECTION_INITIALIZING,
  HTTP_SERVER_CONNECTION_READING_FIRSTLINE,
  HTTP_SERVER_CONNECTION_READING_HEADERS,
  HTTP_SERVER_CONNECTION_READING_BODY,
  HTTP_SERVER_CONNECTION_VALIDATING,
  HTTP_SERVER_CONNECTION_WEATHER_HANDOVER,
  HTTP_SERVER_CONNECTION_RESPONDING,
  HTTP_SERVER_CONNECTION_DISPOSING,
  HTTP_SERVER_CONNECTION_ERROR,

} HTTPServerConnectionState;

typedef struct
{
  HTTPServerConnectionState         state;
  uint8_t                           line_buf[HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN];
  int                               line_buf_len;
  int                               retries; // counter for parsing to avoid infinite loop on lost tcp connection

	void*                             context;
	http_server_connection_on_request on_request;

  Scheduler_Task*                   task;
  TCP_Client                        tcp_client;

  HTTP_Request                      request;
  HTTP_Response                     response;

  int                               weather_done;

} HTTP_Server_Connection;


int http_server_connection_init(HTTP_Server_Connection* _Connection, int _fd);
int http_server_connection_init_ptr(int _fd, HTTP_Server_Connection** _Connection_Ptr);
/* To be called by the dependent module to define what to run when request is made */
void http_server_connection_set_callback(HTTP_Server_Connection* _Connection, void* _Context, http_server_connection_on_request _on_request);

void http_server_connection_dispose(HTTP_Server_Connection* _Connection);
void http_server_connection_dispose_ptr(HTTP_Server_Connection** _Connection_Ptr);


/* ******************************************************************* */
/* ************************** HTTP SERVER **************************** */
/* **** **************************************************************** */
/* This is the main HTTP building block
 * It spawns the TCP Server and a scheduler task for every connection made */

typedef enum {
  HTTP_SERVER_ERROR_NONE = 0,
  HTTP_SERVER_ERROR_INVALID_ARGUMENT, /*Null pointers negative fd's*/
  HTTP_SERVER_ERROR_TCP_INIT_FAILED,
  HTTP_SERVER_ERROR_CREATE_TASK_FAILED,
  HTTP_SERVER_ERROR_ACCEPT_FAILED

}HTTPServerErrorState;

typedef enum {
  HTTP_SERVER_INIT,
  HTTP_SERVER_LISTENING,
  HTTP_SERVER_CONNECTING,
  HTTP_SERVER_CONNECTED,
  HTTP_SERVER_ERROR,
  HTTP_SERVER_DISPOSING

} HTTPServerState;

typedef int (*http_server_on_connection)(void* _Context, HTTP_Server_Connection* _Connection);
typedef int (*http_retry_function)(void *);

typedef struct
{
	http_server_on_connection on_connection;
  void*                     context;
	Scheduler_Task*           task;
	TCP_Server                tcp_server;
  HTTPServerState           state;
  HTTPServerErrorState      error_state;
  int                       client_fd;
  int                       error_retries;
  uint64_t                  next_retry_at;
  http_retry_function       retry_function;
  void*                     retry_args;

} HTTP_Server;

typedef struct {
    TCP_Server *tcp_server;
    const char *port;
    tcp_server_on_accept on_accept;
    void *context; /*HTTP_Server*/
} HTTP_Tcp_Init_Args;

int http_server_init(HTTP_Server* _Server, http_server_on_connection _on_connection, void* _context);
int http_server_init_ptr(http_server_on_connection _on_connection, void* _context, HTTP_Server** _Server_Ptr);

int http_server_on_accept(int _fd, void* _context);
void http_server_dispose(HTTP_Server* _Server);
void http_server_dispose_ptr(HTTP_Server** _Server_Ptr);


#endif //__HTTP_Server_h_
