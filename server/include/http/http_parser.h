#ifndef __HTTP_SERVER_PARSER_H_
#define __HTTP_SERVER_PARSER_H_


/* ******************************************************************* */
/* ************************** HTTP PARSING *************************** */
/* ******************************************************************* */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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


char* http_build_full_response(int _status_code, const char* _reason_phrase, const char* _method, const char* _path, Linked_List* _Headers);

HTTPMethod http_method_string_to_enum(const char* _method_str);
const char* http_method_enum_to_string(HTTPMethod _method);

int http_parse_firstline(HTTP_Request* _Request, const char* _firstline, int _line_len);
int http_parse_headers(const char* _headers_str, int _headers_str_len, Linked_List** _Headers_List_Ptr);

#endif //__HTTP_Server_h_
