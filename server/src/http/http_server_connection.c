#include "../../include/http.h"

#define RESPONSE_TEMPLATE "HTTP/1.1 %i %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s" // args: response_code, reason_phrase, response_content_len, response_body

#define TCP_MESSAGE_BUFFER_SIZE 128

//-----------------Internal Functions-----------------

/* This defines the functions to be called depending on what state the connection is in */
void http_server_connection_taskwork(void* _Context, uint64_t _montime);

//----------------------------------------------------

int http_server_connection_init(HTTP_Server_Connection* _Connection, int _fd)
{

  TCP_Client TCPC;
  _Connection->tcp_client = TCPC;
  _Connection->tcp_client.fd = _fd;
	/* tcp_client_init(&_Connection->tcp_client, _fd); */


	_Connection->task = scheduler_create_task(_Connection, http_server_connection_taskwork);
  _Connection->state = 0;
  /* _Connection->status_code = HttpStatus_Continue; */

	return 0;
}

int http_server_connection_init_ptr(int _fd, HTTP_Server_Connection** _Connection_Ptr)
{
	if(_Connection_Ptr == NULL)
		return -1;

	HTTP_Server_Connection* _Connection = (HTTP_Server_Connection*)malloc(sizeof(HTTP_Server_Connection));
	if(_Connection == NULL)
		return -2;

	int result = http_server_connection_init(_Connection, _fd);
	if(result != 0)
	{
		free(_Connection);
		return result;
	}

	*(_Connection_Ptr) = _Connection;

	return 0;
}

void http_server_connection_set_callback(HTTP_Server_Connection* _Connection, void* _Context, http_server_connection_on_request _on_request)
{
  _Connection->context = _Context;
  _Connection->on_request = _on_request;
}


/* --------------TASKWORK STATE FUNCTIONS-------------- */
HTTPServerConnectionState http_server_connection_work_init(HTTP_Server_Connection* _Connection)
{
  return HTTP_SERVER_CONNECTION_READING_FIRSTLINE;
}
HTTPServerConnectionState http_server_connection_work_request_read_firstline(HTTP_Server_Connection* _Connection)
{
  char* firstline;

  uint8_t buffer[TCP_MESSAGE_BUFFER_SIZE];

  int bytes_read = tcp_client_read_simple(&_Connection->tcp_client, buffer, sizeof(buffer));

  if (bytes_read > 0)
  {
    int i;
    for (i = 1; i < bytes_read; i++)
    {
      if (buffer[i] == '\n' && buffer[i - 1] == '\r')
      {
        memcpy(firstline, buffer, (size_t)i);
        printf("%s", firstline);
        
        //TODO: Parse firstline into Request struct members 
      }
    }
  }

  if (firstline != NULL)
    return HTTP_SERVER_CONNECTION_READING_HEADERS;
  else
    return HTTP_SERVER_CONNECTION_READING_FIRSTLINE;
  

  printf("READ FIRSTLINE\n");
  return HTTP_SERVER_CONNECTION_READING_HEADERS;
}


HTTPServerConnectionState http_server_connection_work_request_read_headers(HTTP_Server_Connection* _Connection)
{
  // Read TCP socket byte4byte to buffer until \r\n\r\request_n


  /* _Connection->request.headers = http_parse_headers();  */


  return HTTP_SERVER_CONNECTION_READING_BODY;
}
HTTPServerConnectionState http_server_connection_work_request_read_body(HTTP_Server_Connection* _Connection)
{

  // Depending on the method we read TCP until 
  // Should have some blockage for too many bytes then it's prob some bullshit

  /* _Connection->request.body = http_parse_body();  */

  return HTTP_SERVER_CONNECTION_RESPONDING;

}
HTTPServerConnectionState http_server_connection_work_respond(HTTP_Server_Connection* _Connection)
{
  /* Just for easier usage*/
  /* HTTP_Request* Request = &_Connection->request;
  HTTP_Response* Response = &_Connection->response;

  const char* reason_phrase = HttpStatus_reasonPhrase(Response->status_code);

  int response_length = strlen(RESPONSE_TEMPLATE) + 
      strlen(reason_phrase) + 
      5; // Max status code length and nullterm
*/



  return HTTP_SERVER_CONNECTION_DISPOSING;
}
/* ---------------------------------------------------- */


void http_server_connection_taskwork(void* _Context, uint64_t _montime)
{
	HTTP_Server_Connection* _Connection = (HTTP_Server_Connection*)_Context;

  switch (_Connection->state)
  {
    case HTTP_SERVER_CONNECTION_INITIALIZING:
    {
      _Connection->state = http_server_connection_work_init(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_READING_FIRSTLINE:
    {
      _Connection->state = http_server_connection_work_request_read_firstline(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_READING_HEADERS:
    {
      _Connection->state = http_server_connection_work_request_read_headers(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_READING_BODY:
    {
      _Connection->state = http_server_connection_work_request_read_body(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_RESPONDING:
    {
      _Connection->state = http_server_connection_work_respond(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_DISPOSING:
    {
      http_server_connection_dispose(_Connection);
    } break;
  }
}

void http_server_connection_dispose(HTTP_Server_Connection* _Connection)
{

	tcp_client_dispose(&_Connection->tcp_client);
	scheduler_destroy_task(_Connection->task);
}

void http_server_connection_dispose_ptr(HTTP_Server_Connection** _Connection_Ptr)
{

	if(_Connection_Ptr == NULL || *(_Connection_Ptr) == NULL)
	  return;

  http_server_connection_dispose(*(_Connection_Ptr));

	free(*(_Connection_Ptr));
	*(_Connection_Ptr) = NULL;

}
