#include "../../include/http.h"
#include <string.h>
#include <stdio.h>

#define RESPONSE_TEMPLATE "HTTP/1.1 %i %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s" // args: response_code, reason_phrase, response_content_len, response_body

//-----------------Internal Functions-----------------

void http_server_connection_taskwork(void* _Context, uint64_t _montime);
HTTPServerConnectionState worktask_init(HTTP_Server_Connection* _Connection);
HTTPServerConnectionState worktask_request_read_firstline(HTTP_Server_Connection* _Connection);
HTTPServerConnectionState worktask_request_read_headers(HTTP_Server_Connection* _Connection);
HTTPServerConnectionState worktask_request_read_body(HTTP_Server_Connection* _Connection);
HTTPServerConnectionState worktask_request_validate(HTTP_Server_Connection* _Connection);
HTTPServerConnectionState worktask_respond(HTTP_Server_Connection* _Connection);

//----------------------------------------------------

int http_server_connection_init(HTTP_Server_Connection* _Connection, int _fd)
{

  TCP_Client TCPC;
  _Connection->tcp_client = TCPC;
  _Connection->tcp_client.fd = _fd;

  _Connection->tcp_client.readData = NULL;
  _Connection->tcp_client.writeData = NULL;
  _Connection->tcp_client.data.addr = NULL;
  _Connection->tcp_client.data.size = 0;
	/* tcp_client_init(&_Connection->tcp_client, _fd); */


	_Connection->task = scheduler_create_task(_Connection, http_server_connection_taskwork);
  _Connection->state = 0;

  _Connection->retries = 0;
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

void http_server_connection_set_callback(HTTP_Server_Connection* _Connection, void* _Context, http_server_connection_on_request _on_request, http_server_connection_on_response _on_response)
{
  _Connection->context = _Context;
  _Connection->on_request = _on_request;
  _Connection->on_response = _on_response;
}


/* --------------TASKWORK STATE FUNCTIONS-------------- */
HTTPServerConnectionState worktask_init(HTTP_Server_Connection* _Connection)
{
  return HTTP_SERVER_CONNECTION_READING_FIRSTLINE;
}
HTTPServerConnectionState worktask_request_read_firstline(HTTP_Server_Connection* _Connection)
{
  if (_Connection->retries > HTTP_SERVER_CONNECTION_MAX_RETRIES)
    return HTTP_SERVER_CONNECTION_ERROR;

  _Connection->retries++;

  if (_Connection->tcp_client.data.addr == NULL)
    _Connection->tcp_client.data.addr = malloc(0);

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];

  int bytes_read = tcp_client_read_simple(&_Connection->tcp_client, tcp_buf, TCP_MESSAGE_BUFFER_MAX_SIZE);
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
          
          printf("Found our newline!line buf: %s\n", _Connection->line_buf);
          
          char buffer[HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN];
          memcpy(buffer, _Connection->line_buf, strlen((char*)_Connection->line_buf));
          
          char* ptr; 

          ptr = strtok((char*)buffer, " ");

          int y;
          for (y = 0; y < 3; y++)
          {
            if (y == 0) // Method
              _Connection->request.method_str = strdup(ptr);
            if (y == 1) // Path+Query
            {
              char* tmp = ptr; // local copy to not ruin ongoing strtok
              char* query = strchr(tmp, '?');
              if (query)
              {
                size_t path_len = (size_t)(query - tmp);
                _Connection->request.path = (char*)malloc(path_len + 1);
                if (_Connection->request.path) 
                {
                  memcpy(_Connection->request.path, tmp, path_len);
                  _Connection->request.path[path_len] = '\0';
                }
                _Connection->request.query = strdup(query);

              } else {
                _Connection->request.path = strdup(ptr);
                _Connection->request.query = NULL;
              }
            }
            if (y == 2) // HTTP Version
            {
              _Connection->request.version = strdup(ptr);
            }

            ptr = strtok(NULL, " ");
          }
          if (!_Connection->request.method_str ||
              !_Connection->request.path       ||
              !_Connection->request.version)
          {
            // We either failed to allocate or there wasn't three parts in firstline
            // Ideally it should distinguish and set httpstatus either 400 or 500
            // Either way we should not continue
            return HTTP_SERVER_CONNECTION_RESPONDING;
          }
          printf("Method: %s\nPath: %s\nVersion: %s\n", _Connection->request.method_str, _Connection->request.path, _Connection->request.version);

          /* Read the last remaining tcp bytes into line buffer */
          for (y = i; y < bytes_read; y++)
          {
            _Connection->line_buf[_Connection->line_buf_len++] = tcp_buf[y];
            /* _Connection->line_buf_len++; */
            printf("Buf read: %c\n", _Connection->line_buf[_Connection->line_buf_len - 1]);
          }

          size_t old_len = 0;
          size_t new_len = _Connection->line_buf_len; 

          if (_Connection->tcp_client.data.addr != NULL) {
            old_len = strlen((char*)_Connection->tcp_client.data.addr);
          }

          uint8_t* new_mem = realloc(_Connection->tcp_client.data.addr, old_len + new_len + 1);
          if (new_mem == NULL) {
            perror("realloc");
            return HTTP_SERVER_CONNECTION_ERROR;
          } else {
            _Connection->tcp_client.data.addr = new_mem;
            memcpy(_Connection->tcp_client.data.addr + old_len, _Connection->line_buf, new_len);
            _Connection->tcp_client.data.addr[old_len + new_len] = '\0';
            _Connection->tcp_client.data.size = strlen((char*)_Connection->tcp_client.data.addr);
          }

          printf("TCP_Data: %s\n", (char*)_Connection->tcp_client.data.addr);
          return HTTP_SERVER_CONNECTION_READING_HEADERS;
        }
        else
        {
          _Connection->request.method = HTTP_INVALID;
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
  //
  return HTTP_SERVER_CONNECTION_READING_FIRSTLINE; // bytes_read exceeded buffer max, we go again
}
HTTPServerConnectionState worktask_request_read_headers(HTTP_Server_Connection* _Connection)
{
  if (_Connection->retries > HTTP_SERVER_CONNECTION_MAX_RETRIES)
    return HTTP_SERVER_CONNECTION_ERROR;

  _Connection->retries++;

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];
  int bytes_read;
  bytes_read = tcp_client_read_simple(&_Connection->tcp_client, tcp_buf, TCP_MESSAGE_BUFFER_MAX_SIZE);
  printf("bytes_read: %i\n", bytes_read);
 
  if (bytes_read > 0)
  {
    size_t old_len = 0;

    if (_Connection->tcp_client.data.addr != NULL) {
      old_len = strlen((char*)_Connection->tcp_client.data.addr);
    }

    uint8_t* new_mem = realloc(_Connection->tcp_client.data.addr, old_len + bytes_read + 1);
    if (new_mem == NULL) {
      perror("realloc");
      return HTTP_SERVER_CONNECTION_ERROR;
    } else {
      _Connection->tcp_client.data.addr = new_mem;
      memcpy(_Connection->tcp_client.data.addr + old_len, tcp_buf, bytes_read);
      _Connection->tcp_client.data.addr[old_len + bytes_read] = '\0';
      _Connection->tcp_client.data.size = old_len + bytes_read + 1;
    }
  }
  printf("TCP_Data addr: %s\n", (char*)_Connection->tcp_client.data.addr);
  printf("TCP_Data size: %zu\n", _Connection->tcp_client.data.size);

  if (_Connection->tcp_client.data.size >= HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN)
  {
    printf("_Connection->tcp_client.data.size too big! %zu\n", _Connection->tcp_client.data.size);
    _Connection->request.method = HTTP_INVALID;
    return HTTP_SERVER_CONNECTION_RESPONDING;
  }

  char buffer[HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN];
  memcpy(buffer, _Connection->tcp_client.data.addr, _Connection->tcp_client.data.size);

  int i;
  for (i = 0; i < _Connection->tcp_client.data.size; i++) 
  {
    printf("loop buf char: %c\n", buffer[i]);
    if (buffer[i] == '\n')
    {
      if (buffer[i - 1] == '\r')
      {
        if (buffer[i - 2] == '\n')
        {
          if (buffer[i - 3] == '\r')
          {
            
            _Connection->request.headers = linked_list_create();
            if (_Connection->request.headers == NULL)
            {
              printf("Failed to create linked list for headers!\n");
              return HTTP_SERVER_CONNECTION_RESPONDING;
            }

            char* ptr; 

            ptr = strtok((char*)buffer, " ");

            printf("ptr: %s\n", ptr);

            int y = 0;
            while (ptr != NULL)
            {
              if (y > 1) // Skip firstline
              {
                printf("Token: %s\n", ptr);
                char* header = strdup(ptr); // NEED TO DISPOSE EACH
                if (header != NULL)
                  linked_list_item_add(_Connection->request.headers, NULL, header);
              }
              ptr = strtok(NULL, "\r\n");
              y++;
            }

            linked_list_foreach(_Connection->request.headers, node)
            {
              printf("header: %s\n", (char*)node->item);
            }

            return HTTP_SERVER_CONNECTION_READING_BODY;
          }
          /* else
          {
            _Connection->request.method = HTTP_INVALID;
            return HTTP_SERVER_CONNECTION_RESPONDING;
          } */
        }
      }
      /* else
      {
        _Connection->request.method = HTTP_INVALID;
        return HTTP_SERVER_CONNECTION_RESPONDING;
      } */
    }
  }


  return HTTP_SERVER_CONNECTION_READING_HEADERS; // bytes_read exceeded buffer max, we go again
}
HTTPServerConnectionState worktask_request_read_body(HTTP_Server_Connection* _Connection)
{
  if (_Connection->retries > HTTP_SERVER_CONNECTION_MAX_RETRIES)
    return HTTP_SERVER_CONNECTION_ERROR;

  _Connection->retries++;

  // Depending on the method we read TCP until 
  // Should have some blockage for too many bytes then it's prob some bullshit

  /* _Connection->request.body = http_parse_body();  */

  return HTTP_SERVER_CONNECTION_VALIDATING;

}
HTTPServerConnectionState worktask_request_validate(HTTP_Server_Connection* _Connection)
{
  _Connection->request.method = http_method_string_to_enum(_Connection->request.method_str);

  HTTP_Request Req = _Connection->request;

  if (Req.method != HTTP_INVALID &&
      Req.path != NULL &&
      Req.version != NULL &&
      Req.headers != NULL)
  {
    _Connection->response.status_code = HttpStatus_OK;
    _Connection->on_request(_Connection->context);

    return HTTP_SERVER_CONNECTION_WEATHER_HANDOVER;
  } 
  else 
  {
    _Connection->response.status_code = HttpStatus_Invalid;

    return HTTP_SERVER_CONNECTION_DISPOSING;
  }

}
HTTPServerConnectionState worktask_respond(HTTP_Server_Connection* _Connection)
{
  HTTP_Request Req = _Connection->request;

  if (strcmp(Req.method_str, "GET") == 0 &&
      Req.path != NULL &&
      Req.version != NULL &&
      Req.headers != NULL)
  {
    _Connection->response.status_code = HttpStatus_OK;
    const char* reason_phrase = HttpStatus_reasonPhrase(_Connection->response.status_code);

    Linked_List* headers = Req.headers;
    size_t headers_size = 0;

    char* full_response = http_build_full_response(_Connection->response.status_code, reason_phrase, Req.method_str, Req.path, Req.headers);
    printf("FULL RESPONSE: \n\n%s\n\n", full_response);

    _Connection->tcp_client.writeData = (char*)malloc(strlen(full_response) + 1);
    if (!_Connection->tcp_client.writeData) {
      free(full_response);
      return HTTP_SERVER_CONNECTION_DISPOSING;
    }
    memcpy(_Connection->tcp_client.writeData, full_response, strlen(full_response));
    _Connection->tcp_client.writeData[strlen(_Connection->tcp_client.writeData)] = '\0';
    free(full_response);
    printf("WRITE DATA: \n\n%s\n\n", _Connection->tcp_client.writeData);

    /* tcp_client_write_simple(&_Connection->tcp_client, (uint8_t*)full_response, strlen(full_response)); */
    tcp_client_write(&_Connection->tcp_client, strlen(_Connection->tcp_client.writeData));
  } 
  else 
  {
    int status_code = 400;
    const char* reason_phrase = "Bad Request";
    const char* response_body = "400 Bad Request";
    int body_len = (int)strlen(response_body);

    int res_len = snprintf(NULL, 0, RESPONSE_TEMPLATE,
                           status_code,
                           reason_phrase,
                           body_len,
                           response_body + 1); 

    char* full_response = (char*)malloc(res_len);
    if (!full_response) 
    {
      perror("malloc");
      return HTTP_SERVER_CONNECTION_DISPOSING;
    }
    printf("reason_phrase: %s\n", reason_phrase);
    printf("status_code: %i\n", _Connection->response.status_code);

    snprintf(full_response, res_len, RESPONSE_TEMPLATE,
             status_code,
             reason_phrase,
             body_len,
             response_body);

    // Allocate writeData buffer for sending
    _Connection->tcp_client.writeData = (char*)malloc(strlen(full_response) + 1);
    if (!_Connection->tcp_client.writeData) {
      free(full_response);
      return HTTP_SERVER_CONNECTION_DISPOSING;
    }
    memcpy(_Connection->tcp_client.writeData, full_response, strlen(full_response));
    _Connection->tcp_client.writeData[strlen(_Connection->tcp_client.writeData)] = '\0';
    free(full_response);
    printf("WRITE DATA: \n\n%s\n\n", _Connection->tcp_client.writeData);
    tcp_client_write(&_Connection->tcp_client, res_len); 
  }

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
      _Connection->state = worktask_init(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_READING_FIRSTLINE:
    {
      printf("HTTP_SERVER_CONNECTION_READING_FIRSTLINE\n");
      _Connection->state = worktask_request_read_firstline(_Connection);
    } break;
    

    case HTTP_SERVER_CONNECTION_READING_HEADERS:
    {
      printf("HTTP_SERVER_CONNECTION_READING_HEADERS\n");
      _Connection->state = worktask_request_read_headers(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_READING_BODY:
    {
      printf("HTTP_SERVER_CONNECTION_READING_BODY\n");
      _Connection->state = worktask_request_read_body(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_VALIDATING:
    {
      printf("HTTP_SERVER_CONNECTION_VALIDATING\n");
      _Connection->state = worktask_request_validate(_Connection);
    }

    case HTTP_SERVER_CONNECTION_WEATHER_HANDOVER:
    {
      if (_Connection->weather_done != 0)
        _Connection->state = HTTP_SERVER_CONNECTION_RESPONDING;
    } break;
      
    case HTTP_SERVER_CONNECTION_RESPONDING:
    {
      printf("HTTP_SERVER_CONNECTION_RESPONDING\n");
      _Connection->state = worktask_respond(_Connection);
    } break;

    case HTTP_SERVER_CONNECTION_DISPOSING:
    {
      printf("HTTP_SERVER_CONNECTION_DISPOSING\n");
      _Connection->on_response(_Connection->context);

    } break;

    case HTTP_SERVER_CONNECTION_ERROR:
    {
      printf("HTTP_SERVER_CONNECTION_ERROR\n");
      _Connection->response.status_code = 500;
      _Connection->state = HTTP_SERVER_CONNECTION_RESPONDING;
      
    } break;

  }
}

void http_server_connection_dispose(HTTP_Server_Connection* _Connection)
{

  tcp_client_dispose(&_Connection->tcp_client);

  /* Free TCP_Data */
  if (_Connection->tcp_client.data.addr != NULL)
  {
    free(_Connection->tcp_client.data.addr);
    _Connection->tcp_client.data.addr = NULL;
  }

  /* Free HTTP_Request */
  if (_Connection->request.method_str != NULL)
  {
    free(_Connection->request.method_str);
    _Connection->request.method_str = NULL;
  }
  if (_Connection->request.path != NULL)
  {
    free(_Connection->request.path);
    _Connection->request.path = NULL;
  }
  if (_Connection->request.query != NULL)
  {
    free(_Connection->request.query);
    _Connection->request.query = NULL;
  }
  if (_Connection->request.version != NULL)
  {
    free(_Connection->request.version);
    _Connection->request.version = NULL;
  }


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
