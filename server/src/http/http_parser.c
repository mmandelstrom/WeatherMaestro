#include "../../include/http.h"

int http_server_parse_request_string(const char* _request_str, HTTP_Request* _Request)
{

  
  return 0;
}

http_server_parser_dispose(HTTP_Server_Connection* _Connection)
{

  if (_Connection->response != NULL)
    free((*_ConnectionPtr)->method);
  free(_Connection)
}
