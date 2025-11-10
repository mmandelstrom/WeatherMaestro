#include "../../include/http.h"
#include <string.h>
#include <stdio.h>

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

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];

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
          
          _Connection->line_buf[i] = tcp_buf[i];
          printf("Found our newline!line buf: %s\n", _Connection->line_buf);


          //TODO: Parse firstline into Request struct members 
          
          char buffer[HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN];
          
          /* sscanf((char*)_Connection->line_buf, "%[^ ]", buffer);
          Req.method_str = strdup(buffer);
          
          sscanf((char*)_Connection->line_buf, "%*[^ ] %[^ ]", buffer);
          Req.path = strdup(buffer);


          sscanf((char*)_Connection->line_buf, "%*[^ ] %*[^ ] %[^\r]", buffer);
          Req.version = strdup(buffer); */


          
          char* ptr; 
          ptr = strtok((char*)_Connection->line_buf, " ");

          int y;
          for (y = 0; y < 3; y++)
          {
            if (y == 0)
              Req.method_str = strdup(ptr);
            if (y == 1)
              Req.path = strdup(ptr);
            if (y == 2)
              Req.version = strdup(ptr);

            ptr = strtok(NULL, " ");
          }
          printf("Method: %s\nPath: %s\nVersion: %s\n", Req.method_str, Req.path, Req.version);

          
          /* Read the last remaining tcp bytes into line buffer */
          for (y = i; y < bytes_read; y++)
          {
            _Connection->line_buf[y-1] = tcp_buf[y-1];
            _Connection->line_buf_len++;
            printf("Buf read: %c\n", _Connection->line_buf[y-1]);
          }

          printf("Found our newline!line buf: %s\n", _Connection->line_buf);
          return HTTP_SERVER_CONNECTION_READING_HEADERS;
        }
        else
        {
          Req.method = HTTP_INVALID;
          return HTTP_SERVER_CONNECTION_RESPONDING;
        }
      }
      else
      {	//Continue inserting each byte into buffer 
        _Connection->line_buf[i-1] = tcp_buf[i-1];
        _Connection->line_buf_len++;
        printf("Buf read: %c\n", _Connection->line_buf[i-1]);
      }
    }

  }

  /* if (Conn_Data->size > HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN)
  {
    printf("Connection (fd: %i) had a too long first line, closing.", _Connection->tcp_client.fd);
        return HTTP_SERVER_CONNECTION_RESPONDING;
  } */

  //if firstline parsed
  /* return HTTP_SERVER_CONNECTION_READING_HEADERS; */
  //else
  return HTTP_SERVER_CONNECTION_READING_FIRSTLINE; // bytes_read exceeded buffer max, we go again
}
HTTPServerConnectionState http_server_connection_work_request_read_headers(HTTP_Server_Connection* _Connection)
{

  /* memset(_Connection->line_buf, 0, sizeof(_Connection->line_buf)); */

  HTTP_Request Req = _Connection->request;

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];

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
          if (tcp_buf[i - 2] == '\n')
          {
            if (tcp_buf[i - 3] == '\r')
            {
              printf("Found our headers: %s\n", _Connection->line_buf);

              Req.headers = linked_list_create();
              char buffer[HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN];

              char* ptr; 
              ptr = strtok((char*)_Connection->line_buf, "\r\n");
              printf("ptr: %s\n", ptr);

              while (ptr != NULL)
              {
                printf("Token: %s\n", ptr);
                char* header = strdup(ptr); // NEED TO DISPOSE EACH
                if (header != NULL)
                {
                  linked_list_item_add(Req.headers, NULL, header);
                }
                ptr = strtok(NULL, "\r\n");
              }

              linked_list_foreach(Req.headers, node)
              {
                printf("header: %s\n", (char*)node->item);
              }


              Req.method_str = strdup(buffer);


              //TODO: Parse firstline into Request struct members 


              return HTTP_SERVER_CONNECTION_RESPONDING;
            }
            else
            {
              Req.method = HTTP_INVALID;
              return HTTP_SERVER_CONNECTION_RESPONDING;
            }
          }
        }
        else
        {
          Req.method = HTTP_INVALID;
          return HTTP_SERVER_CONNECTION_RESPONDING;
        }
      }
      else
      {	//Continue inserting each byte into buffer 
        _Connection->line_buf[i-1] = tcp_buf[i-1];
        _Connection->line_buf_len++;
        printf("Buf read: %c\n", _Connection->line_buf[i-1]);
      }
    }

  }

  return HTTP_SERVER_CONNECTION_READING_HEADERS; // bytes_read exceeded buffer max, we go again

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
      5; // Max status code length and nullterm */




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
    default:
    {
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
