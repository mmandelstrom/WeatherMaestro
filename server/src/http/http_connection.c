#include "../../include/http/http_connection.h"

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

  TCP_Client *TCPC = (TCP_Client*)calloc(1, sizeof(TCP_Client));
  HTTP_Request *req = (HTTP_Request*)calloc(1, sizeof(HTTP_Request));
  HTTP_Response *resp = (HTTP_Response*)calloc(1, sizeof(HTTP_Response));
  if (!TCPC || !req || !resp) {
    perror("calloc");
    return -1;
  }
  
  _Connection->tcp_client = TCPC;
  _Connection->tcp_client->fd = _fd;
  _Connection->tcp_client->data.addr = calloc(1, sizeof(uint8_t));
  if (!_Connection->tcp_client->data.addr) {
    perror("calloc");
    free(TCPC);
    free(req);
    free(resp);
    return -1;
  }
  _Connection->request = req;
  _Connection->response = resp;
	_Connection->task = scheduler_create_task(_Connection, http_server_connection_taskwork);

  _Connection->state = HTTP_SERVER_CONNECTION_INITIALIZING;
  _Connection->retries = 0;
  _Connection->weather_done = 0;

	return 0;
}

int http_server_connection_init_ptr(int _fd, HTTP_Server_Connection** _Connection_Ptr)
{
	if(_Connection_Ptr == NULL) {
    return -1;
  }

  HTTP_Server_Connection* _Connection = calloc(1, sizeof(HTTP_Server_Connection));
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

/*From weatherinstance "init"*/
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
  if (_Connection->retries++ > HTTP_SERVER_CONNECTION_MAX_RETRIES) {
    /*Create internal error enum and set here*/
    return HTTP_SERVER_CONNECTION_ERROR;
  }

  TCP_Client *tcpc = _Connection->tcp_client;

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];
  int bytes_read = tcp_client_read_simple(tcpc, tcp_buf, TCP_MESSAGE_BUFFER_MAX_SIZE);
  printf("Bytes read: %d\n", bytes_read);

  if (bytes_read > 0) {

    ssize_t bytes_stored = tcp_client_realloc_data(&tcpc->data, tcp_buf, (size_t)bytes_read);
    if (bytes_stored < 0) {
      /*Add internal error*/
      return HTTP_SERVER_CONNECTION_ERROR;
    }
  }

  if (tcpc->data.size == 0) {
    /*No data yet, try again on next work call*/
    return HTTP_SERVER_CONNECTION_READING_FIRSTLINE;
  }

  int line_end = http_parser_find_line_end(tcpc->data.addr, tcpc->data.size);
  if (line_end < 0) {
    /*No \r\n found yet*/
    if (tcpc->data.size >= HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN) {
      /*Invalid request*/
      printf("Request too large..\n");
      _Connection->response->status_code = 400;
      return HTTP_SERVER_CONNECTION_RESPONDING;
    }
    /*Keep looking for line end on next work call*/
    return HTTP_SERVER_CONNECTION_READING_FIRSTLINE;
  }

  /*line_end is the index of the first \r*/
  size_t line_len = (size_t)line_end;

  if (line_len == 0 || line_len >= HTTP_SERVER_CONNECTION_FIRSTLINE_MAXLEN) {
    printf("Request too large..\n");
    _Connection->response->status_code = 400;
    return HTTP_SERVER_CONNECTION_RESPONDING;
  }

  /*line_buf for debugging*/
  memset(_Connection->line_buf, 0, sizeof(_Connection->line_buf));
  memcpy(_Connection->line_buf, tcpc->data.addr, line_len);
  _Connection->line_buf[line_len] = '\0';
  _Connection->line_buf_len = (int)line_len;
  _Connection->request->firstline_len = line_len;

  printf("First line found\r\nline_buf_len: %d\r\nline_buf: %s\n", _Connection->line_buf_len, (char*)_Connection->line_buf);


  if (http_parser_first_line((const char*)_Connection->line_buf, line_len, _Connection->request) != 0) {
    /*Add internal error*/
    return HTTP_SERVER_CONNECTION_ERROR;
  }

  printf("Method: %d\n", _Connection->request->method);
  printf("Method_str: %s\n", _Connection->request->method_str);
  printf("Version: %s\n", _Connection->request->version);

  /*We have handled first line + 2 for \r\n*/
  size_t parsed = line_len + 2;

  /*If there is data remaining after first line shift it to beggining of buffer*/
  if (tcpc->data.size > parsed) {

    memmove(tcpc->data.addr,
            tcpc->data.addr + parsed,
            tcpc->data.size - parsed);
  }

  /*Remove first line by shrinking the buffer*/
  tcpc->data.size -= parsed;

  return HTTP_SERVER_CONNECTION_READING_HEADERS;

}

HTTPServerConnectionState worktask_request_read_headers(HTTP_Server_Connection* _Connection)
{
  if (_Connection->retries++ > HTTP_SERVER_CONNECTION_MAX_RETRIES) {
    /*Add internal error*/
    return HTTP_SERVER_CONNECTION_ERROR; 
  }
  
  TCP_Client *tcpc = _Connection->tcp_client;

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];
  int bytes_read = tcp_client_read_simple(tcpc, tcp_buf, TCP_MESSAGE_BUFFER_MAX_SIZE);
  
  if (bytes_read > 0) {
    ssize_t bytes_stored = tcp_client_realloc_data(&tcpc->data,
                                                  tcp_buf,
                                                  (size_t)bytes_read);

    if (bytes_stored < 0) {
      /*add internal error*/
      return HTTP_SERVER_CONNECTION_ERROR;
    }
  }

  if (tcpc->data.size == 0) {
    /*No data, try again on next work call*/
    return HTTP_SERVER_CONNECTION_READING_HEADERS;
  }

  /*Edgecase no headers*/
  if (tcpc->data.size >= 2 &&
      tcpc->data.addr[0] == '\r' &&
      tcpc->data.addr[1] == '\n') {
    if (tcpc->data.size > 2) {
      memmove(tcpc->data.addr, tcpc->data.addr + 2, tcpc->data.size -2);
    }
    tcpc->data.size -= 2;

    /*Create empty header so we can dispose without crash*/
    _Connection->request->headers = linked_list_create();
    if (!_Connection->request->headers) {
      /*add internal error*/
      return HTTP_SERVER_CONNECTION_ERROR;
    }

    _Connection->retries = 0;
    return HTTP_SERVER_CONNECTION_VALIDATING;
  }

  int headers_end = http_parser_find_headers_end(tcpc->data.addr,
                                                 tcpc->data.size);

  if (headers_end < 0) {
    /*Continue reading on next work call*/
    return HTTP_SERVER_CONNECTION_READING_HEADERS;
  }

  /*headers_end is the index of the first \r parser will ignore last line*/
  size_t header_len = (size_t)headers_end + 2;
  
  /*We have still parsed the full line including \r\n\r\n*/
  size_t parsed = (size_t)headers_end + 4;

  if (http_parser_headers((const char*)tcpc->data.addr,
                          header_len,
                          &_Connection->request->headers) !=0) {
    _Connection->response->status_code = 400;
    return HTTP_SERVER_CONNECTION_RESPONDING;
  }

  /*If there is data remaining its the body move it to start of buffer*/
  if (tcpc->data.size > parsed) {
    memmove(tcpc->data.addr,
            tcpc->data.addr + parsed,
            tcpc->data.size - parsed);
  }

  tcpc->data.size -= parsed;
  _Connection->retries = 0;

  /*Check if there is a content-length (body) to read*/
  const char* content_length_string = NULL;
  int result = http_parser_get_header_value(_Connection->request->headers,
                                            "Content-Length", &content_length_string);
  if (result < 0) {
    printf("Content-Length header not found\n");
    return HTTP_SERVER_CONNECTION_VALIDATING;
  }

  int content_length = atoi(content_length_string);
  _Connection->content_length = content_length;

  if (content_length > 0) {
    /*There is a body to read*/
    return HTTP_SERVER_CONNECTION_READING_BODY;
  }

  return HTTP_SERVER_CONNECTION_VALIDATING;
}

HTTPServerConnectionState worktask_request_read_body(HTTP_Server_Connection* _Connection)
{
  if (_Connection->retries++ > HTTP_SERVER_CONNECTION_MAX_RETRIES)
    return HTTP_SERVER_CONNECTION_ERROR;
  
  TCP_Client *tcpc = _Connection->tcp_client;

  uint8_t tcp_buf[TCP_MESSAGE_BUFFER_MAX_SIZE];
  int bytes_read = tcp_client_read_simple(tcpc,
                                          tcp_buf,
                                          TCP_MESSAGE_BUFFER_MAX_SIZE);

  if (bytes_read > 0) {
    ssize_t bytes_stored = tcp_client_realloc_data(&tcpc->data,
                                                  tcp_buf,
                                                  (size_t)bytes_read);
                                                  

    if (bytes_stored < 0) {
      /*Add internal error*/
      return HTTP_SERVER_CONNECTION_ERROR;
    }
  }

  if (tcpc->data.size < (size_t)_Connection->content_length) {
    /*Keep reading body on next work call*/
    return HTTP_SERVER_CONNECTION_READING_BODY;
  }
  printf("EXPECTED: %d, HAVE: %zu\n", _Connection->content_length, tcpc->data.size);
  _Connection->retries = 0;
  return HTTP_SERVER_CONNECTION_VALIDATING;
}

HTTPServerConnectionState worktask_request_validate(HTTP_Server_Connection* _Connection)
{
  HTTP_Request *req = _Connection->request;
  if (!req->method_str || !req->path || !req->version) {
    _Connection->response->status_code = 400;
    return HTTP_SERVER_CONNECTION_RESPONDING;
  }

  if (req->method == HTTP_INVALID) {
    _Connection->response->status_code = 400;
    return HTTP_SERVER_CONNECTION_RESPONDING;
  }

  _Connection->response->status_code = 200;
  return HTTP_SERVER_CONNECTION_RESPONDING;
 }

HTTPServerConnectionState worktask_respond(HTTP_Server_Connection* _Connection)
{
  TCP_Client *tcpc = _Connection->tcp_client;
  HTTP_Request *req = _Connection->request;

  char *body_ptr = NULL;
  int body_len = 0;

  /*Get body if there is one*/
  if (_Connection->content_length > 0 &&
      tcpc->data.size >= (size_t)_Connection->content_length) {
    body_ptr = (char*)tcpc->data.addr;
    body_len = _Connection->content_length;
  }

  /*Write headers*/
  char headers_json[1024];
  headers_json[0] = '\0';

  if (req->headers) {
    linked_list_foreach(req->headers, node) {
      HTTP_Header *h = (HTTP_Header*)node->item;
      char temp[128];
      snprintf(temp, sizeof(temp),
               "    { \"key\": \"%s\", \"value\": \"%s\" }, \n",
               h->key, h->value);
      strncat(headers_json, temp, sizeof(headers_json) - strlen(headers_json) -1);
    }
  }

  /*Remove trailing ,*/
  size_t headers_len = strlen(headers_json);
  if (headers_len > 2 && headers_json[headers_len -2] == ',') {
    headers_json[headers_len - 2] = '\n';
    headers_json[headers_len - 1] = '\0';
  }

  /*Build response*/

  char response_body[2048];
  snprintf(response_body, sizeof(response_body),
             "{\n"
             "  \"method\": \"%s\",\n"
             "  \"path\": \"%s\",\n"
             "  \"query\": \"%s\",\n"
             "  \"headers\": [\n%s  ],\n"
             "  \"body\": \"%.*s\"\n"
             "}\n",
             req->method_str,
             req->path,
             req->query ? req->query : "",
             headers_json,
             body_len,
             body_ptr ? body_ptr : "");

  int response_body_len = strlen(response_body);

  char http_response[4096];
  int written = snprintf(
      http_response, sizeof(http_response),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: %d\r\n"
      "Connection: close\r\n"
      "\r\n"
      "%s",
      response_body_len,
      response_body
  );

  tcpc->writeData = malloc(written + 1);
  if (!tcpc->writeData) {
    perror("malloc");
    /*Add internal error*/
    return HTTP_SERVER_CONNECTION_ERROR;
  }

  memcpy(tcpc->writeData, http_response, written);
  tcpc->writeData[written] = '\0';

  tcp_client_write(tcpc, written);

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
      tcp_client_disconnect(_Connection->tcp_client);
      _Connection->on_response(_Connection->context);

    } break;

    case HTTP_SERVER_CONNECTION_ERROR:
    {
      printf("HTTP_SERVER_CONNECTION_ERROR\n");
      _Connection->response->status_code = 500;
      _Connection->state = HTTP_SERVER_CONNECTION_RESPONDING;
      
    } break;

  }
}

void http_server_connection_dispose(HTTP_Server_Connection* _Connection)
{
  tcp_client_dispose(_Connection->tcp_client);
  if (_Connection->tcp_client != NULL)
  {
    free(_Connection->tcp_client);
    _Connection->tcp_client = NULL;
  }

  /* Free TCP_Data */
  /* if (_Connection->tcp_client->data.addr != NULL)
  {
    free(_Connection->tcp_client->data.addr);
    _Connection->tcp_client->data.addr = NULL;
    _Connection->tcp_client->data.size = 0;
  } */

  /* Free HTTP_Request */
  if (_Connection->request != NULL)
  {
    if (_Connection->request->method_str != NULL)
    {
      free(_Connection->request->method_str);
      _Connection->request->method_str = NULL;
    }
    if (_Connection->request->path != NULL)
    {
      free(_Connection->request->path);
      _Connection->request->path = NULL;
    }
    if (_Connection->request->query != NULL)
    {
      free(_Connection->request->query);
      _Connection->request->query = NULL;
    }
    if (_Connection->request->version != NULL)
    {
      free(_Connection->request->version);
      _Connection->request->version = NULL;
    }
    if (_Connection->request->params != NULL)
    {
      int i;
      for (i = 0; i < _Connection->request->params_count; i++)
      {
        if (_Connection->request->params[i].key != NULL) {
          free(_Connection->request->params[i].key);
          _Connection->request->params[i].key = NULL;
        }
        if (_Connection->request->params[i].val != NULL)
        {
          free(_Connection->request->params[i].val);
          _Connection->request->params[i].val = NULL;
        }
      }
      free(_Connection->request->params);
      _Connection->request->params_count = 0;
    }

    http_parser_dispose_headers(_Connection->request->headers);

    _Connection->request->headers = NULL;
    free(_Connection->request);
    _Connection->request = NULL;
  }

  /* Free HTTP_Response */
  if (_Connection->response != NULL)
  {
    if (_Connection->response->body != NULL)
    {
      free(_Connection->response->body);
      _Connection->response->body = NULL;
    }

    free(_Connection->response);
    _Connection->response = NULL;
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

#ifdef HTTP_FUZZ_MODE

void http_handle_request(const uint8_t *_data, size_t _len) {

  HTTP_Server_Connection conn;
  memset(&conn, 0, sizeof(HTTP_Server_Connection));

  TCP_Client tcpc;
  memset(&tcpc, 0, sizeof(TCP_Client));

  HTTP_Request req;
  memset(&req, 0, sizeof(HTTP_Request));

  HTTP_Response resp;
  memset(&resp, 0, sizeof(HTTP_Response));

  conn.tcp_client = &tcpc;
  conn.request = &req;
  conn.response = &resp;

  conn.state = HTTP_SERVER_CONNECTION_READING_FIRSTLINE;
  conn.retries = 0;
  conn.weather_done = 1;
  conn.context = NULL;
  conn.on_request = NULL;
  conn.on_response = NULL;
  conn.task = NULL;

  /*Fake tcp socket*/
  tcpc.fd = -1;
  tcpc.data.addr = malloc(_len);
  if (!tcpc.data.addr) {
    perror("malloc");
    return;
  }

  memcpy(tcpc.data.addr, _data, _len);
  tcpc.data.size = _len;

  int guard = 0;
  while (guard++ < 100 &&
         conn.state != HTTP_SERVER_CONNECTION_DISPOSING &&
         conn.state != HTTP_SERVER_CONNECTION_ERROR) {
    http_server_connection_taskwork(&conn, 0);
  }


  /*Cleanup*/

  if (tcpc.writeData) {
    free(tcpc.writeData);
    tcpc.writeData = NULL;
  }

  if (tcpc.data.addr){
    free(tcpc.data.addr);
    tcpc.data.addr = NULL;
    tcpc.data.size = 0;
  }

  if (req.method_str) {
    free(req.method_str);
    req.method_str = NULL;
  }

  if (req.path) {
    free(req.path);
    req.path = NULL;
  }
  
  if (req.query) {
    free(req.query);
    req.query = NULL;
  }

  if (req.version) {
    free(req.version);
    req.version = NULL;
  }
  
  if (req.headers) {
    linked_list_foreach(req.headers, node) {
      if (node->item) {
        free(node->item);
        node->item = NULL;
      }
    }
    linked_list_items_dispose(req.headers);
    linked_list_destroy(&req.headers);
    req.headers = NULL;
  }

  if (resp.body) {
    free(resp.body);
    resp.body = NULL;
  }
}

#endif /*HTTP_FUZZ_MODE*/
