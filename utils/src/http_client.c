#include "../include/http_client.h"
#include <stdio.h>

void http_client_taskwork(void* _context, uint64_t _montime);
HTTPClientState http_client_worktask_connecting(HTTP_Client* _Client);
HTTPClientState http_client_worktask_build_request(HTTP_Client* _Client);
HTTPClientState http_client_worktask_send_request(HTTP_Client* _Client);
HTTPClientState http_client_worktask_read_firstline(HTTP_Client* _Client);
HTTPClientState http_client_worktask_read_headers(HTTP_Client* _Client);
HTTPClientState http_client_worktask_read_body(HTTP_Client* _Client);
HTTPClientState http_client_worktask_returning(HTTP_Client* _Client);


int http_client_initiate(HTTP_Client* _Client, const char* _URL, HTTPMethod _method) {
  if (!_Client) {
    return ERR_INVALID_ARG;
  }
  HTTP_Request* req = calloc(1, sizeof(HTTP_Request));
  if (!req){
    return ERR_NO_MEMORY;
  }

  HTTP_Response* resp = calloc(1, sizeof(HTTP_Response));
  if (!resp){
    return ERR_NO_MEMORY;
  }



  _Client->task = scheduler_create_task(_Client, http_client_taskwork);
  if (!_Client->task) {
    free(req);
    return ERR_BUSY;
  }
  _Client->resp = resp;
  _Client->req = req;
  _Client->URL = _URL;
  _Client->method = _method;
  _Client->state = HTTP_CLIENT_CONNECTING;
  _Client->request_length = 0;
  _Client->bytes_sent = 0;
  _Client->retries = 0;
  _Client->next_retry_at = SystemMonotonicMS();


  //Check url for http/https 
  _Client->tls = false;
  
  return 0;

}

HTTPClientState http_client_worktask_connecting(HTTP_Client* _Client) {
  if (!_Client) {
    return HTTP_CLIENT_ERROR;
  }

  if (_Client->tls == false) {
    const char* PORT = "443";
    TCP_Client* TCPC = calloc(1, sizeof(TCP_Client));
    _Client->tcp_client = TCPC;

    int result = tcp_client_init(_Client->tcp_client, _Client->URL, PORT); 

    if (result != SUCCESS) {
      return HTTP_CLIENT_ERROR;
    }

    return HTTP_CLIENT_BUILD_REQUEST;
  } else {
    //init tls client etc
    return HTTP_CLIENT_BUILD_REQUEST;
  }
 
  return HTTP_CLIENT_BUILD_REQUEST;
}

HTTPClientState http_client_worktask_build_request(HTTP_Client* _Client) {
  if (!_Client) {
    return HTTP_CLIENT_ERROR;
  }
  _Client->request_buffer = malloc(256);
  if (!_Client->request_buffer) {
    return HTTP_CLIENT_ERROR;
  }

  if (_Client->method == HTTP_GET) {
  _Client->request_length = snprintf((char*)_Client->request_buffer, 256, "GET %s HTTP/1.1\r\nHost: stockholm2.onvo.se\r\nAgent: httpclient\r\nConnection: close\r\n\r\n", _Client->URL);
  if (_Client->request_length < 0){

      return HTTP_CLIENT_ERROR;
    }
  }
  
  return HTTP_CLIENT_SEND_REQUEST;
}

HTTPClientState http_client_worktask_send_request(HTTP_Client* _Client) {

  if (!_Client) {
    return HTTP_CLIENT_ERROR;
  }
  if (_Client->tls == false) {
    size_t remaining = 0;
    remaining = _Client->request_length - _Client->bytes_sent;

    int written = tcp_client_write_simple(_Client->tcp_client, _Client->request_buffer + _Client->bytes_sent, remaining);

    if (written > 0) {
      _Client->bytes_sent += written;
      if (_Client->bytes_sent >= _Client->request_length) {

        _Client->retries = 0;
        return HTTP_CLIENT_READ_FIRSTLINE;
      }

      _Client->next_retry_at = SystemMonotonicMS() + 100;
      return HTTP_CLIENT_SEND_REQUEST;

    }else if (written == 0) {
      _Client->retries++;
      _Client->next_retry_at = SystemMonotonicMS() + 1000;

      return HTTP_CLIENT_SEND_REQUEST;

    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        _Client->next_retry_at = SystemMonotonicMS() + 100;
        return HTTP_CLIENT_SEND_REQUEST;
      }
      if (_Client->retries < 3) {
        _Client->retries++;
        _Client->next_retry_at = SystemMonotonicMS() + 1000;
        return HTTP_CLIENT_SEND_REQUEST;
      }
      return HTTP_CLIENT_ERROR;
    }
  } else {
    //implement tls read here
    return HTTP_CLIENT_READ_FIRSTLINE;
  }

  _Client->retries = 0;
  return HTTP_CLIENT_READ_FIRSTLINE;
}

HTTPClientState http_client_worktask_read_firstline(HTTP_Client* _Client) {
  if (!_Client) {
    return HTTP_CLIENT_ERROR;
  }
  if (_Client->retries++ > 3) {
    /*Create internal error enum and set here*/
    return HTTP_CLIENT_ERROR;
  }

  TCP_Client* TCP_C = _Client->tcp_client;

  uint8_t tcp_buf[1024];
  int bytes_read = tcp_client_read_simple(TCP_C, tcp_buf, 1024);
  printf("Bytes read: %d\n", bytes_read);

  if (bytes_read > 0) {
    ssize_t bytes_stored = tcp_client_realloc_data(&TCP_C->data, tcp_buf, (size_t)bytes_read);
    if (bytes_stored < 0) {
      /*Add internal error*/
      return HTTP_CLIENT_ERROR;
    }
  }

  if (TCP_C->data.size == 0) {
    /*No data yet, try again on next work call*/
    return HTTP_CLIENT_READ_FIRSTLINE;
  }

  int line_end = http_parser_find_line_end(TCP_C->data.addr, TCP_C->data.size);
  if (line_end < 0) {
    /*No \r\n found yet*/
    if (TCP_C->data.size >= 1024) {
      /*Invalid request*/
      printf("Request too large..\n");
      return HTTP_CLIENT_ERROR;
    }
    /*Keep looking for line end on next work call*/
    return HTTP_CLIENT_READ_FIRSTLINE;
  }

  /*line_end is the index of the first \r*/
  size_t line_len = (size_t)line_end;

  if (line_len == 0 || line_len >= 1024) {
    printf("Request too large..\n");
    return HTTP_CLIENT_ERROR;
  }
  
  if (http_parser_response_firstline((const char*)TCP_C->data.addr, TCP_C->data.size, _Client->resp) != SUCCESS) {
    /*Add internal error*/
    return HTTP_CLIENT_ERROR;
  }

  printf("Version: %s\n", _Client->resp->version);
  printf("status_code_string: %s\n", _Client->resp->status_code_string);
  printf("reason_phrase: %s\n", _Client->resp->reason_phrase);

  /*We have handled first line + 2 for \r\n*/
  size_t parsed = line_len + 2;

  /*If there is data remaining after first line shift it to beggining of buffer*/
  if (TCP_C->data.size > parsed) {

    memmove(TCP_C->data.addr,
            TCP_C->data.addr + parsed,
            TCP_C->data.size - parsed);
  }

  /*Remove first line by shrinking the buffer*/
  TCP_C->data.size -= parsed;

  return HTTP_CLIENT_READ_HEADERS;

}


HTTPClientState http_client_worktask_read_headers(HTTP_Client* _Client) {
  if (_Client->retries++ > 3) {
    /*Add internal error*/
    return HTTP_CLIENT_ERROR; 
  }


  if (_Client->req->params != NULL) {
    printf("Printing params...\n");
    linked_list_foreach(_Client->req->params, node) {
      HTTP_Key_Value *p = (HTTP_Key_Value*)node->item;
      printf("ParamKey: %s\nParamValue: %s\n", p->key, p->value);
    }
  }

  
  TCP_Client *TCP_C = _Client->tcp_client;

  uint8_t tcp_buf[1024];
  int bytes_read = tcp_client_read_simple(TCP_C, tcp_buf, 1024);
  
  if (bytes_read > 0) {
    ssize_t bytes_stored = tcp_client_realloc_data(&TCP_C->data,
                                                  tcp_buf,
                                                  (size_t)bytes_read);

    if (bytes_stored < 0) {
      /*add internal error*/
      return HTTP_CLIENT_ERROR;
    }
  }

  if (TCP_C->data.size == 0) {
    /*No data, try again on next work call*/
    return HTTP_CLIENT_READ_HEADERS;
  }

  /*Edgecase no headers*/
  if (TCP_C->data.size >= 2 &&
      TCP_C->data.addr[0] == '\r' &&
      TCP_C->data.addr[1] == '\n') {
    if (TCP_C->data.size > 2) {
      memmove(TCP_C->data.addr, TCP_C->data.addr + 2, TCP_C->data.size -2);
    }
    TCP_C->data.size -= 2;

    /*Create empty header so we can dispose without crash*/
    _Client->req->headers = linked_list_create();
    if (!_Client->req->headers) {
      /*add internal error*/
      return HTTP_CLIENT_ERROR;
    }

    _Client->retries = 0;
    return HTTP_CLIENT_ERROR;
  }

  int headers_end = http_parser_find_headers_end(TCP_C->data.addr,
                                                 TCP_C->data.size);

  if (headers_end < 0) {
    /*Continue reading on next work call*/
    return HTTP_CLIENT_READ_HEADERS;
  }

  /*headers_end is the index of the first \r parser will ignore last line*/
  size_t header_len = (size_t)headers_end + 4;
  
  /*We have still parsed the full line including \r\n\r\n*/
  size_t parsed = (size_t)headers_end + 4;

  if (http_parser_headers((const char*)TCP_C->data.addr,
                          header_len,
                          &_Client->req->headers) != SUCCESS) {
    return HTTP_CLIENT_ERROR;
  }

  /*If there is data remaining its the body move it to start of buffer*/
  if (TCP_C->data.size > parsed) {
    memmove(TCP_C->data.addr,
            TCP_C->data.addr + parsed,
            TCP_C->data.size - parsed);
  }

  TCP_C->data.size -= parsed;
  _Client->retries = 0;

  /*Check if there is a content-length (body) to read*/
  const char* content_length_string = NULL;
  int result = http_parser_get_header_value(_Client->req->headers,
                                            "Content-Length", &content_length_string);
  if (result < 0) {
    printf("Content-Length header not found\n");
    return HTTP_CLIENT_RETURNING;
  }

  int content_length = atoi(content_length_string);
  _Client->content_length = content_length;

  if (content_length > 0) {
    /*There is a body to read*/
    return HTTP_CLIENT_READ_BODY;
  }

  return HTTP_CLIENT_RETURNING;

}

HTTPClientState http_client_worktask_read_body(HTTP_Client* _Client) {
  if (!_Client) {
    return HTTP_CLIENT_ERROR;
  }
  
  if (_Client->retries++ > 3)
    return HTTP_CLIENT_ERROR;
  
  TCP_Client *TCP_C = _Client->tcp_client;

  uint8_t tcp_buf[1024];
  int bytes_read = tcp_client_read_simple(TCP_C,
                                          tcp_buf,
                                          1024);

  if (bytes_read > 0) {
    ssize_t bytes_stored = tcp_client_realloc_data(&TCP_C->data,
                                                  tcp_buf,
                                                  (size_t)bytes_read);
                                                  

    if (bytes_stored < 0) {
      /*Add internal error*/
      return HTTP_CLIENT_ERROR;
    }
  }

  if (TCP_C->data.size < (size_t)_Client->content_length) {
    /*Keep reading body on next work call*/
    return HTTP_CLIENT_READ_BODY;
  }

  printf("EXPECTED: %d, HAVE: %zu\n", _Client->content_length, TCP_C->data.size);
  _Client->retries = 0;
  return HTTP_CLIENT_RETURNING;
}

HTTPClientState http_client_worktask_returning(HTTP_Client* _Client) {
  printf("print from worktask returning: %s\n", _Client->tcp_client->data.addr);

  return HTTP_CLIENT_DISPOSING;
}


void http_client_taskwork(void* _context, uint64_t _montime) {
  
  if (!_context) {
    return;
  }
  (void)_montime;
  HTTP_Client* client = (HTTP_Client*)_context;

  uint64_t now = SystemMonotonicMS();

  switch(client->state) {
    case HTTP_CLIENT_INITIALIZING: {
      printf("init state\n");
      break;
    }
    case HTTP_CLIENT_CONNECTING: {
      client->state = http_client_worktask_connecting(client);
      break;
    }
    case HTTP_CLIENT_BUILD_REQUEST: {
      client->state = http_client_worktask_build_request(client);
      break;
    }
    case HTTP_CLIENT_SEND_REQUEST: {
      if (now >= client->next_retry_at) {
        client->state = http_client_worktask_send_request(client);
        break;
      }
      break;
    }
    case HTTP_CLIENT_READ_FIRSTLINE: {
      if (now >= client->next_retry_at) {
        client->state = http_client_worktask_read_firstline(client);
        break;
      }
      break;
    }
    case HTTP_CLIENT_READ_HEADERS: {
       if (now >= client->next_retry_at) {
        client->state = http_client_worktask_read_headers(client);
        break;
      }
      break;
    }
    case HTTP_CLIENT_READ_BODY: {
      if (now >= client->next_retry_at) {
        client->state = http_client_worktask_read_body(client);
        break;
      }
      break;
    }
     case HTTP_CLIENT_RETURNING: {
      client->state = http_client_worktask_returning(client);
      break;
    }
    case HTTP_CLIENT_ERROR: {
      printf("Error\n");
      break;
    }
    case HTTP_CLIENT_DISPOSING: {
      printf("Disposing\n");
      break;
    }
    default: {
      printf("default\n");
      break;
    }
  }
}
void http_client_dispose(HTTP_Client* _Client);

