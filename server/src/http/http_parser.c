#include "../../include/http/http_parser.h"

#define RESPONSE_TEMPLATE "HTTP/1.1 %i %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s" // args: response_code, reason_phrase, response_content_len, response_body

#define RESPONSE_BODY_TEMPLATE \
  "{\n" \
  "  \"method\": \"%s\",\n" \
  "  \"path\": \"%s\",\n" \
  "  \"headers\": [\n%s\n  ]\n" \
  "}"

int http_parse_firstline(HTTP_Request* _Request, const char* _firstline, int _line_len)
{
  printf(" -----------Firstline:\n%s\n", _firstline);
  char* ptr; 
  char line_copy[_line_len];
  memcpy(line_copy, _firstline, strlen(_firstline) +1);
  ptr = strtok(line_copy, " ");

  int j;
  for (j = 0; j < 3; j++)
  {
    if (j == 0 && ptr != NULL) // Method
    {
      printf("Method ptr: %s\n", ptr);
      _Request->method_str = strdup(ptr);
    }
    if (j == 1 && ptr != NULL) // Path+Query
    {
      char* tmp = ptr; // local copy to not ruin ongoing strtok
      char* query = strchr(tmp, '?');
      if (query != NULL)
      {
        size_t path_len = (size_t)(query - tmp);
        _Request->path = (char*)malloc(path_len + 1);
        if (_Request->path) 
        {
          memcpy(_Request->path, tmp, path_len);
          _Request->path[path_len] = '\0';
        }
        _Request->query = strdup(query + 1); // + 1 to strip '?'

      } else {
        _Request->path = strdup(ptr);
        _Request->query = NULL;
      }
    }
    if (j == 2 && ptr != NULL) // HTTP Version
    {
      
      printf("Request version: %s\n", ptr);
      _Request->version = strdup(ptr);
    }

    ptr = strtok(NULL, " ");
  }
  if (!_Request->method_str ||
      !_Request->path       ||
      !_Request->version)
  {
    // We either failed to allocate or there wasn't three parts in firstline
    // Ideally it should distinguish and set httpstatus either 400 or 500
    // Either way we should not continue
    return -1;
  }
  printf("Method: %s\nPath: %s\nVersion: %s\n", _Request->method_str, _Request->path, _Request->version);

  /* Query parsing using yuarel */
  if (_Request->query != NULL)
  {
    printf("Query: %s\n", _Request->query);

    yuarel_param Params_Buf[HTTP_REQUEST_MAX_PARAMS + 1];
    _Request->params_count = yuarel_parse_query(_Request->query, '&', Params_Buf, HTTP_REQUEST_MAX_PARAMS + 1);
    printf("Params count: %i\n", _Request->params_count);

    if (_Request->params_count < 0)
    {
      printf("yuarel failed to parse request query\r\n");
      //server error
      return -2;
    } 
    else if (_Request->params_count > HTTP_REQUEST_MAX_PARAMS)
    {
      printf("request had too many query params\r\n");
      //invalid request
      return -3;
    }
    else // Allocate memory for our request params
    {
      _Request->params = malloc(_Request->params_count * sizeof(yuarel_param));
      if (_Request->params == NULL)
      {
        perror("malloc");
        return -4;
      }

      for (j = 0; j < _Request->params_count; j++)
      {
        /* Param key */
        size_t key_len = strlen(Params_Buf[j].key);
        _Request->params[j].key = (char*)malloc(key_len + 1);
        if (_Request->params[j].key == NULL)
        {
          perror("malloc");
          for (int k = 0; k < j; k++) // gracefully dispose of allocated mem so far
          {
            free(_Request->params[k].key);
            free(_Request->params[k].val);
          }
          free(_Request->params);
          return -5;
        }
        memcpy(_Request->params[j].key, Params_Buf[j].key, key_len + 1);

        /* Param value */
        if (Params_Buf[j].val != NULL) // Handle empty params
        {
          size_t val_len = strlen(Params_Buf[j].val);
          _Request->params[j].val = (char*)malloc(val_len + 1);
          if (_Request->params[j].val == NULL)
          {
            perror("malloc");
            for (int k = 0; k <= j; k++) // gracefully dispose of allocated mem so far
            {
              free(_Request->params[k].key); // remove one extra key
              if (k < j) free(_Request->params[k].val);
            }
            free(_Request->params);
            return -6;
          }
          memcpy(_Request->params[j].val, Params_Buf[j].val, val_len + 1);
        }
      }
    }
  }
  return 0;
}

int http_parse_headers(const char* _headers_str, int _headers_str_len, Linked_List** _Headers_List_Ptr)
{
  *_Headers_List_Ptr = linked_list_create();
  if (*_Headers_List_Ptr == NULL)
  {
    printf("Failed to create linked list for headers!\n");
    return -1;
  }

  char headers_copy[_headers_str_len];
  memcpy(headers_copy, _headers_str, _headers_str_len);

  char* ptr; 
  ptr = strtok(headers_copy, "\r\n");

  int y = 0;
  while (ptr != NULL)
  {
    char* header = strdup(ptr); // NEED TO DISPOSE EACH
    if (header == NULL)
    {
      perror("strdup");
      return -2;
    }
    else linked_list_item_add(*_Headers_List_Ptr, NULL, header);
  
    ptr = strtok(NULL, "\r\n");
    y++;
  }

  linked_list_foreach(*_Headers_List_Ptr, node)
  {
    printf("Header: %s\n", (char*)node->item);
  }

  return 0;
}

char* http_build_full_response(int _status_code, const char* _reason_phrase, const char* _method, const char* _path, Linked_List* _Headers) {

  return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 0\r\nConnection: close\r\n\r\n" "";

}

HTTPMethod http_method_string_to_enum(const char* _method_str)
{

  if (strcmp(_method_str, "GET") == 0)
    return HTTP_GET;
  if (strcmp(_method_str, "OPTIONS") == 0)
    return HTTP_OPTIONS;
  if (strcmp(_method_str, "POST") == 0)
    return HTTP_POST;
  if (strcmp(_method_str, "PUT") == 0)
    return HTTP_PUT;
  if (strcmp(_method_str, "DELETE") == 0)
    return HTTP_DELETE;
  if (strcmp(_method_str, "DOWNLOAD") == 0)
    return HTTP_DOWNLOAD;

  return HTTP_INVALID;

}
const char* http_method_enum_to_string(HTTPMethod _method)
{

  if (_method == HTTP_OPTIONS)
    return "OPTIONS";
  if (_method == HTTP_GET)
    return "GET";
  if (_method == HTTP_POST)
    return "POST";
  if (_method == HTTP_PUT)
    return "PUT";
  if (_method == HTTP_DELETE)
    return "DELETE";
  if (_method == HTTP_DOWNLOAD)
    return "DOWNLOAD";
  if (_method == HTTP_INVALID)
    return "INVALID";

  return NULL;
}
