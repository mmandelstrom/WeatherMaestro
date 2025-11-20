#ifndef __HTTP_SERVER_PARSER_H_
#define __HTTP_SERVER_PARSER_H_


/* ******************************************************************* */
/* ************************** HTTP PARSING *************************** */
/* ******************************************************************* */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../../../libs/include/HTTPStatusCodes.h"
#include "../../../libs/include/yuarel.h"
#include "../../../utils/include/utils.h"
#include "../../../utils/include/linked_list.h"

#define HTTP_REQUEST_MAX_PARAMS 10

typedef enum
{
  HTTP_OPTIONS,
  HTTP_GET,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_DOWNLOAD,

  HTTP_INVALID

} HTTPMethod;

typedef struct
{
  enum HttpStatus_Code status_code;

  char*                head;
  char*                headers;
  char*                body;  

} HTTP_Response;

typedef struct
{
  HTTPMethod      method;
  char*           method_str;
  char*           path; 
  char*           query; // maybe we don't need to save this now that we have array of params
  yuarel_param*   params;
  int             params_count;
  char*           version;
  int             firstline_len; // To set pointer for headers parsing
  int             headers_len;
  int             bode_len;
  Linked_List*    headers;

  char*           body;

} HTTP_Request;

int http_parser_first_line(const char *_line, size_t _line_len, HTTP_Request* _Req);
int http_parser_find_line_end(const uint8_t *_buf, size_t _buf_len, size_t _start_offset);
int http_parser_find_headers_end(const uint8_t *_buf, size_t _buf_len, size_t _start_offset);
int http_parser_headers(const char *_buf, size_t _buf_len, Linked_List **_headers_out);

#endif //__HTTP_Server_h_
