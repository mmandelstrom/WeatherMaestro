#include "../include/http_client.h"
#include <sys/types.h>

void http_client_taskwork(void* _context, uint64_t _montime);
HTTPClientState http_client_worktask_connecting(HTTP_Client* _Client);
HTTPClientState http_client_worktask_build_request(HTTP_Client* _Client);
HTTPClientState http_client_worktask_send_request(HTTP_Client* _Client);
HTTPClientState http_client_worktask_receive_response(HTTP_Client* _Client);



int http_client_initiate(HTTP_Client* _Client, http_client_on_success _on_success, const char* _URL, HTTPMethod _method) {
  if (!_Client || !_on_success) {
    return ERR_INVALID_ARG;
  }
  
  _Client->on_success = _on_success;

  _Client->task = scheduler_create_task(_Client, http_client_taskwork);
  if (!_Client->task) {
    return ERR_BUSY;
  }
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
int http_client_get(HTTP_Client* _Client, const char* _URL, void (*callback)(HTTP_Client* _CLient, const char* _Event));
void http_client_dispose(HTTP_Client* _Client);

HTTPClientState http_client_worktask_connecting(HTTP_Client* _Client) {
  if (!_Client) {
    return HTTP_CLIENT_ERROR;
  }

  if (_Client->tls == false) {
    const char* PORT = "443";
    TCP_Client* TCPC;
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
  if (!_Client) {
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
  
    _Client->bytes_sent += tcp_client_write_simple(_Client->tcp_client, _Client->request_buffer, _Client->request_length);

    if (_Client->bytes_sent == _Client->request_length) {
      _Client->retries = 0;
      return HTTP_CLIENT_RECEIVE_RESPONSE;

    }else if (_Client->bytes_sent < _Client->request_length && _Client->retries < 3) {
      _Client->retries++;
      _Client->next_retry_at = SystemMonotonicMS() + 1000;

      return HTTP_CLIENT_SEND_REQUEST;

    } else {
      return HTTP_CLIENT_ERROR;
    }
  } else {
    //implement tls read here
    return HTTP_CLIENT_RECEIVE_RESPONSE;
  }

  _Client->retries = 0;
  return HTTP_CLIENT_RECEIVE_RESPONSE;
}

HTTPClientState http_client_worktask_receive_response(HTTP_Client* _Client) {
  if (!_Client) {
    return HTTP_CLIENT_ERROR;
  }
  
  /*Sno hela skiten från http_connection så slipper vi parsa igen*/
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
    case HTTP_CLIENT_RECEIVE_RESPONSE: {
      printf("Receive\n");
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

