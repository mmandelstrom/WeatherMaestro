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


int http_find_headers_end(const uint8_t *_buf, size_t _buf_len, size_t _start_offset) {
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
      continue;
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

    /*ADD TO LINKED LIST HERE!!!!!!*/

    free(line);
    start = line_end + 2;
  }
  
  return 0;
}
