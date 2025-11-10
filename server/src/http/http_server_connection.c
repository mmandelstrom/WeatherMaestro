#include "../../include/http.h"

#define RESPONSE_TEMPLATE "HTTP/1.1 %i %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s" // args: response_code, reason_phrase, response_content_len, response_body

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
  HTTP_Request Req = _Connection->request;
  TCP_Data* Conn_Data = &_Connection->tcp_client.data;
  if (Conn_Data->addr == NULL) // If it's null we allocate it an address
  {
    Conn_Data->addr = malloc(1);
    Conn_Data->size = 0;
    if (Conn_Data->addr == NULL)
    {
      perror("malloc");
      return HTTP_SERVER_CONNECTION_ERROR;
    }
  }

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];
  uint8_t line_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];

  int bytes_read = tcp_client_read_simple(&_Connection->tcp_client, tcp_buf, sizeof(tcp_buf));
  printf("bytes_read: %i\n", bytes_read);
 
  if (bytes_read > 0)
  {
    int i;
    for (i = 1; i < bytes_read; i++) // Will this create bug with \n on last buffer index?
    {
      if (tcp_buf[i] == '\n')
      {
        if (tcp_buf[i - 1] == '\r')
        {
          
          printf("Found our newline!\n");
          //TODO: Parse firstline into Request struct members 
        }
        else
        {
          Req.method = HTTP_INVALID;
          return HTTP_SERVER_CONNECTION_RESPONDING;
        }
      }
      else
      {	//Continue inserting each byte into buffer 
        line_buf[i] = tcp_buf[i-1];
        printf("Buf read: %c\n", Conn_Data->addr[Conn_Data->size -1]);
      }
    }

    size_t bytes_written = tcp_client_read_buffer_to_data_struct(Conn_Data, (void*)Conn_Data->addr, Conn_Data->size, sizeof(char)); // Write 
    printf("bytes_written: %i\nData addr: %s\n\nData size: %zu\n", bytes_read, Conn_Data->addr, Conn_Data->size);
    if (bytes_written <= 0)
    {
      free(Conn_Data->addr);
      return HTTP_SERVER_CONNECTION_ERROR;
    }

  }

  if (Conn_Data->size > HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN)
  {
    printf("Connection (fd: %i) had a too long first line, closing.", _Connection->tcp_client.fd);
    /* Should send response fail reason something like "Your request head was too long" */
    return HTTP_SERVER_CONNECTION_RESPONDING;
  }

  //if firstline parsed
  return HTTP_SERVER_CONNECTION_READING_HEADERS;
  //else
  return HTTP_SERVER_CONNECTION_READING_FIRSTLINE; // bytes_read exceeded buffer max, we go again
}
HTTPServerConnectionState http_server_connection_work_request_read_headers(HTTP_Server_Connection* _Connection)
{
  // Read TCP socket byte4byte to buffer until \r\n\r\n


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
  HTTP_Request* Request = &_Connection->request;
  HTTP_Response* Response = &_Connection->response;

  const char* reason_phrase = HttpStatus_reasonPhrase(Response->status_code);

  int response_length = strlen(RESPONSE_TEMPLATE) + 
      strlen(reason_phrase) + 
      5; // Max status code length and nullterm




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
      printf("HTTP_SERVER_CONNECTION_READING_FIRSTLINE\n");
      _Connection->state = http_server_connection_work_request_read_firstline(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_READING_HEADERS:
    {
      printf("HTTP_SERVER_CONNECTION_READING_HEADERS\n");
      _Connection->state = http_server_connection_work_request_read_headers(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_READING_BODY:
    {
      printf("HTTP_SERVER_CONNECTION_READING_BODY\n");
      _Connection->state = http_server_connection_work_request_read_body(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_RESPONDING:
    {
      printf("HTTP_SERVER_CONNECTION_READING_RESPONDING\n");
      _Connection->state = http_server_connection_work_respond(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_DISPOSING:
    {
      printf("HTTP_SERVER_CONNECTION_READING_DISPOSING\n");
      http_server_connection_dispose(_Connection);
    } break;
  }
}

void http_server_connection_dispose(HTTP_Server_Connection* _Connection)
{

  /* TODO: free TCP_Data */
	/* tcp_client_dispose(&_Connection->tcp_client); */
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
