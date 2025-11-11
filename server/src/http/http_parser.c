#include "../../include/http.h"

#define RESPONSE_TEMPLATE "HTTP/1.1 %i %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s" // args: response_code, reason_phrase, response_content_len, response_body

#define RESPONSE_BODY_TEMPLATE \
  "{\n" \
  "  \"method\": \"%s\",\n" \
  "  \"path\": \"%s\",\n" \
  "  \"headers\": [\n%s\n  ]\n" \
  "}"

// Example function to build the response body JSON string:
char* build_response_body(const char* _method, const char* _path, Linked_List* _headers) {
  // First compute headers_json size
  size_t headers_size = 0;
  int header_count = 0;

  linked_list_foreach(_headers, node) {
    char* header_str = (char*)node->item;
    // Escape quotes in header string with worst case doubling of quotes plus padding
    size_t len = strlen(header_str);
    size_t esc_len = len;
    for (size_t i = 0; i < len; i++) {
      if (header_str[i] == '"') {
        esc_len++; // one extra for backslash
      }
    }
    // Add length for indentation (4 spaces), quotes (2), comma and newline (2), total 8 extra per header
    headers_size += esc_len + 8;
    header_count++;
  }
  if (header_count == 0) headers_size = 0;

  char* headers_json = malloc(headers_size + 1);
  if (!headers_json) return NULL;
  headers_json[0] = '\0';

  int i = 0;
  linked_list_foreach(_headers, node) {
    char* header_str = (char*)node->item;
    // escape quotes
    char header_escaped[2048]; // large buffer to safely store escaped string
    int j = 0;
    for (size_t k = 0; header_str[k] && j + 2 < sizeof(header_escaped); k++) {
      if (header_str[k] == '"') {
        header_escaped[j++] = '\\';
      }
      header_escaped[j++] = header_str[k];
    }
    header_escaped[j] = '\0';

    strcat(headers_json, "    \"");
    strcat(headers_json, header_escaped);
    strcat(headers_json, "\"");

    if (i != header_count - 1) {
      strcat(headers_json, ",\n");
    } else {
      strcat(headers_json, "\n");
    }
    i++;
  }

  // Calculate required buffer size for full response body JSON string
  int needed = snprintf(NULL, 0, RESPONSE_BODY_TEMPLATE, _method, _path, headers_json);
  if (needed < 0) {
    free(headers_json);
    return NULL;
  }

  size_t buf_size = (size_t)needed + 1;
  char* response_body = malloc(buf_size);
  if (!response_body) {
    free(headers_json);
    return NULL;
  }

  int written = snprintf(response_body, buf_size, RESPONSE_BODY_TEMPLATE, _method, _path, headers_json);
  free(headers_json);

  if (written < 0 || written >= (int)buf_size) {
    free(response_body);
    return NULL;
  }

  return response_body;
}

char* build_full_response(int _status_code, const char* _reason_phrase, const char* _method, const char* _path, Linked_List* _Headers) {
    char* body = build_response_body(_method, _path, _Headers);
    if (!body) return NULL;

    size_t body_len = strlen(body);
    size_t resp_buf_size = strlen(RESPONSE_TEMPLATE) + body_len + 64;
    char* response = malloc(resp_buf_size);
    if (!response) {
      free(body);
      return NULL;
    }

    int ret = snprintf(response, resp_buf_size, RESPONSE_TEMPLATE, _status_code, _reason_phrase, (int)body_len, body);
    free(body);

    if (ret < 0 || ret >= (int)resp_buf_size) {
      free(response);
      return NULL;
    }

    return response;
}
