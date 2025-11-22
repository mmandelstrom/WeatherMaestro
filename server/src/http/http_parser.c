#include "../../include/http/http_parser.h"

int http_parser_first_line(const char *_line, size_t _line_len, HTTP_Request* _Req) {
  if (!_line || !_Req || _line_len < 1) {
    errno = EINVAL;
    return -1;
  }

  char *line_copy = malloc(_line_len + 1);
  if (!line_copy) {
    perror("malloc");
    return -1;
  }

  memcpy(line_copy, _line, _line_len);
  line_copy[_line_len] = '\0';

  char* method = line_copy;
  char *first_space = strchr(line_copy, ' ');
  if (!first_space) {
    free(line_copy);
    return -1;
  }
  *first_space = '\0';
  
  char *request_target = first_space + 1;
  char *second_space = strchr(request_target, ' ');
  if (!second_space) {
    free(line_copy);
    return -1;
  }
  *second_space = '\0';

  char *version = second_space + 1;
  if (*version == 0) {
    free(line_copy);
    return -1;
  }

  char *question_mark = strchr(request_target, '?');
  if (question_mark) {
    *question_mark = '\0';
    _Req->path = strdup(request_target);
    _Req->query = strdup(question_mark + 1);
  } else {
    _Req->path = strdup(request_target);
    _Req->query = NULL;
  }

  _Req->method_str = strdup(method);
  _Req->version = strdup(version);
  _Req->method = http_method_string_to_enum(_Req->method_str);

  free(line_copy);
  
  return 0;
}

int http_parser_find_line_end(const uint8_t *_buf, size_t _buf_len, size_t _start_offset) {
  if (_buf_len < 2) {
    errno = EINVAL;
    return -1;
  }
  
  size_t i;
  for (i = _start_offset; i + 1 < _buf_len; i++) {
    if (_buf[i] == '\r' && _buf[i + 1] == '\n') {
      return (int)i;
    }
  }
  return -1;
}


int http_parser_find_headers_end(const uint8_t *_buf, size_t _buf_len, size_t _start_offset) {
  if (_buf_len < 4) {
    return -1;
  }
  size_t i;
  for (i = _start_offset; i + 3 < _buf_len; i++) {
    if (_buf[i] == '\r' &&
        _buf[i+1] == '\n' &&
        _buf[i+2] == '\r' &&
        _buf[i+3] == '\n') {
      return (int)i;
    }
  }
  return -1;
}

int http_parser_headers(const char *_buf, size_t _buf_len, Linked_List **_headers_out) {
  if (!_buf || !_headers_out) {
    errno = EINVAL;
    return -1;
  }

  *_headers_out = linked_list_create();
  if (!*(_headers_out)) {
    return -1;
  }

  size_t start = 0;
  while (start < _buf_len) {

    int line_end = -1;
    size_t i;

    for (i = start; i + 1 < _buf_len; i++) {
      if (_buf[i] == '\r' && _buf[i+1] == '\n') {
        line_end = (int)i;
        break;
      }
    }

    if (line_end < 0) 
      break;

    size_t line_len = (size_t)line_end - start;
    
    if (line_len == 0) {
      start = line_end + 2;
      break;
    }
    
    char *line = (char*)malloc(line_len + 1);
    if (!line) {
      perror("malloc");
      return -1;
    }

    memcpy(line, _buf + start, line_len);
    line[line_len] = '\0';

    char *colon = strchr(line, ':');
    if (!colon) {
      free(line);
      return -1;
    }

    *colon = '\0';
    char *key = line;
    char *value = colon + 1;

    /*Ignore leading whitespace or tabs by moving pointer forward*/
    while (*value == ' ' || *value == '\t') {
      value++;
    }

    /*Remove trailing whitespace from key*/
    /*end is '\0' so we want to look for the char before it*/
    char *end = key +(strlen(key));
    while (end > key &&
      (end[-1] == ' ' ||
        end[-1] == '\t' ||
        end[-1] == '\r' ||
        end[-1] == '\n')) {

      *--end = '\0';
    }

    /*Same thing for value, remove whitespace or unwanted chars*/
    end = value + strlen(value);
    while (end > value &&
      (end[-1] == ' ' ||
        end[-1] == '\t' ||
        end[-1] == '\r' ||
        end[-1] == '\n')) {
      *--end = '\0';
    }

    HTTP_Header *header = (HTTP_Header*)malloc(sizeof(HTTP_Header));
    if (!header) {
      perror("malloc");
      free(line);
      return -1;
    }

    header->key = strdup(key);
    header->value = strdup(value);

    if(!header->key || !header->value) {
      perror("strdup");
      free(header->key);
      free(header->value);
      free(header);
      free(line);
      return -1;
    }

    linked_list_item_add(*(_headers_out), NULL, header);

    free(line);
    start = line_end + 2;
  }
  
  return 0;
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

int http_parser_get_header_value(Linked_List* _headers, char* _name, const char** _out_value) {
  
  if (!_headers || !_name || !_out_value) {
    return -1;
  }

  linked_list_foreach(_headers, node) {
    HTTP_Header *h = (HTTP_Header*)node->item;
    if (!h || !h->key || !h->value) {
      continue;
    }
    
    if (strcmp(h->key, _name) == 0) {
      *(_out_value) = h->value;
      return 0;
    }
  }

  return -1;
}

void http_parser_dispose_headers(Linked_List *_headers) {
  if (!_headers) return;

  linked_list_foreach(_headers, node) {
    HTTP_Header *header = (HTTP_Header*)node->item;
    if (header) {
      free(header->key);
      free(header->value);
      free(header);
    }
  }

  linked_list_items_dispose(_headers);
  linked_list_destroy(&_headers);      
}

