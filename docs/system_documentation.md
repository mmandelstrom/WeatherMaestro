# WeatherMaestro System Documentation  
---

## System Overview

The system consists of three cooperating servers plus a set of helper components:

- **TCP Server** – Listens on a network port, accepts clients, and passes accepted sockets to the HTTP Server.  
- **HTTP Server** – Owns the listening TCP server, manages accepted sockets, and spawns per-connection HTTP state machines.  
- **Weather Server** – Owns the HTTP server, manages weather-specific instances, and coordinates weather API logic.  

Supporting components:

- **TCP Client** – Thin wrapper for reading/writing on a connected socket.  
- **HTTP Parser** – Parses HTTP request line, headers, and simple helpers.  
- **Weather API** – Translates HTTP requests (paths/params) into weather operations.  
- **Meteo API** – Talks to Open-Meteo (HTTP client via libcurl-like interface).  
- **Weather Parser** – Converts Meteo API responses into `Weather` structs + JSON.  
- **Scheduler** – Cooperative task scheduler driving all state machines.  

Each server and each connection/instance is driven by the global **scheduler** via task callbacks like:
- `tcp_server_taskwork()`, `http_server_taskwork()`, `weather_server_taskwork()`
- `http_server_connection_taskwork()`, `weather_server_instance_taskwork()`

---

## 1. TCP Server

The TCP server listens for incoming connections and hands off accepted sockets via a callback.

### 1.1 States (`TCPServerState`)

| State                  | Meaning                                                                 |
|------------------------|-------------------------------------------------------------------------|
| `TCP_SERVER_INIT`      | Server object created, not yet listening.                              |
| `TCP_SERVER_LISTENING` | Listening socket created; waiting for `accept()`.                      |
| `TCP_SERVER_CONNECTING`| A client fd has just been accepted; executing handover callback.       |
| `TCP_SERVER_CONNECTED` | Connection successfully handed over; server returns to `LISTENING`.    |
| `TCP_SERVER_ERROR`     | A fatal error occurred (e.g., bind/listen/accept failure).             |
| `TCP_SERVER_DISPOSING` | Cleaning up scheduler task and internal resources.                     |

### 1.2 State Flow (Normal Operation)

```text
TCP_SERVER_INIT
   ↓
TCP_SERVER_LISTENING
   ├─ accept() OK        → TCP_SERVER_CONNECTING
   │                         └─ handover OK → TCP_SERVER_CONNECTED → TCP_SERVER_LISTENING
   ├─ accept() EAGAIN…   → (stay in TCP_SERVER_LISTENING)
   └─ fatal error        → TCP_SERVER_ERROR → TCP_SERVER_DISPOSING
```

### 1.3 Function Return Summaries

- `int tcp_server_set_nonblocking(int fd)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_IO`

- `int tcp_server_init(TCP_Server* server, const char* port, tcp_server_on_accept on_accept, void* context)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_IO`

- `int tcp_server_init_ptr(TCP_Server** server_ptr, const char* port, tcp_server_on_accept on_accept, void* context)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, plus any from `tcp_server_init`

- `int tcp_server_accept(TCP_Server* server)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_WOULD_BLOCK`, `ERR_IO`, `ERR_CONNECTION_FAIL`

- `void tcp_server_taskwork(void* context, uint64_t montime)`  
  - Drives state machine, no return value.

- `TCPServerState tcp_server_handle_listening(TCP_Server* server, uint64_t montime)`  
  - **Returns:** `TCP_SERVER_CONNECTING`, `TCP_SERVER_LISTENING`, or `TCP_SERVER_ERROR`

- `TCPServerState tcp_server_connection_handover(TCP_Server* server)`  
  - **Returns:** `TCP_SERVER_CONNECTED` or `TCP_SERVER_ERROR`

- `void tcp_server_dispose(TCP_Server* server)`  
  
- `void tcp_server_dispose_ptr(TCP_Server** server_ptr)`  

---

## 2. TCP Client

The TCP client wraps a connected socket and provides read/write helpers.

### 2.1 Function Return Summaries

- `int tcp_client_init(TCP_Client* client, const char* host, const char* port)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_IO`

- `int tcp_client_init_ptr(TCP_Client** client_ptr, const char* host, const char* port)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, plus any from `tcp_client_init`

- `int tcp_client_set_nonblocking(int fd)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_IO`

- `ssize_t tcp_client_realloc_data(TCP_Data* data, void* input, size_t size)`  
  - **Success:** number of bytes appended  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`

- `int tcp_client_read_simple(TCP_Client* client, uint8_t* buf, int buf_len)`  
  - **Success:** `>= 0` bytes read  
  - **Failure:** `-1` from `recv`

- `int tcp_client_read(TCP_Client* client)`  
  - **Success:** `SUCCESS` (no data) or `> 0` bytes read  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, `ERR_IO`

- `int tcp_client_write(TCP_Client* client, size_t length)`  
  - **Success:** `>= 0` bytes written  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_IO`

- `int tcp_client_write_simple(TCP_Client* client, const uint8_t* buf, int len)`  
  - **Success:** `>= 0` bytes written  
  - **Failure:** `-1` from `send`

- `void tcp_client_disconnect(TCP_Client* client)`  
  
- `void tcp_client_dispose(TCP_Client* client)`  
  
- `void tcp_client_dispose_ptr(TCP_Client** client_ptr)`  

---

## 3. HTTP Server

The HTTP server owns a TCP server, manages per-connection objects and retry logic.

### 3.1 States (`HTTPServerState`)

| State                   | Meaning                                                                              |
|-------------------------|--------------------------------------------------------------------------------------|
| `HTTP_SERVER_INIT`      | HTTP server created; TCP init in progress or pending.                               |
| `HTTP_SERVER_IDLE`      | Ready; waiting for new accepted clients.                                            |
| `HTTP_SERVER_CONNECTING`| A client fd is available; performing connection handover to `HTTP_Server_Connection`. |
| `HTTP_SERVER_CONNECTED` | Handover completed; returns to `IDLE`.                                              |
| `HTTP_SERVER_ERROR`     | TCP or accept initialization error; may trigger retry logic.                        |
| `HTTP_SERVER_DISPOSING` | Cleaning up HTTP and TCP resources, shutting down.                                  |

### 3.2 HTTP Error States (`HTTPServerErrorState`)

- `HTTP_SERVER_ERROR_NONE`  
- `HTTP_SERVER_ERROR_INVALID_ARGUMENT`  
- `HTTP_SERVER_ERROR_ACCEPT_FAILED`  
- `HTTP_SERVER_ERROR_TCP_INIT_FAILED`  

### 3.3 Function Return Summaries

- `int http_server_init(HTTP_Server* http, http_server_on_connection cb, void* context)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, `ERR_FATAL`, plus any from `tcp_server_init`

- `int http_server_initiate_ptr(http_server_on_connection cb, void* context, HTTP_Server** server_ptr)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, plus any from `http_server_init`

- `int http_server_on_accept(int fd, void* context)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`

- `HTTPServerState http_server_connection_handover(int fd, void* context)`  
  - **Returns:** `HTTP_SERVER_CONNECTED`, `HTTP_SERVER_IDLE`, or `HTTP_SERVER_ERROR`

- `void http_server_taskwork(void* context, uint64_t montime)`  
  
- `int http_retry_tcp_init(void* args)`  
  - **Success:** `SUCCESS`  
  - **Failure:** propagated error from `tcp_server_init`

- `HTTPServerState http_server_error_work(HTTP_Server* server)`  
  - **Returns:** `HTTP_SERVER_IDLE`, `HTTP_SERVER_ERROR`, or `HTTP_SERVER_DISPOSING`

- `HTTPServerState http_server_retry_work(HTTP_Server* server)`  
  - **Returns:** `HTTP_SERVER_IDLE`, `HTTP_SERVER_ERROR`, or `HTTP_SERVER_DISPOSING`

- `void http_server_dispose(HTTP_Server* server)`  
  
- `void http_server_dispose_ptr(HTTP_Server** server_ptr)`  

---

## 4. HTTP Connection

Each accepted socket is wrapped in an `HTTP_Server_Connection` with its own state machine.

### 4.1 Connection States (`HTTPServerConnectionState`)

| State                                   | Meaning                                                                      |
|-----------------------------------------|------------------------------------------------------------------------------|
| `HTTP_SERVER_CONNECTION_INITIALIZING`   | Connection created; about to begin reading request.                          |
| `HTTP_SERVER_CONNECTION_READING_FIRSTLINE` | Reading and parsing the HTTP request line.                             |
| `HTTP_SERVER_CONNECTION_READING_HEADERS`   | Reading and parsing HTTP headers.                                       |
| `HTTP_SERVER_CONNECTION_READING_BODY`      | Reading request body according to `Content-Length`.                     |
| `HTTP_SERVER_CONNECTION_VALIDATING`        | Validating parsed request; calling `on_request` callback.               |
| `HTTP_SERVER_CONNECTION_WEATHER_HANDOVER`  | Waiting for Weather instance to finish (`weather_done`).                |
| `HTTP_SERVER_CONNECTION_RESPONDING`        | Building and sending HTTP response to client.                           |
| `HTTP_SERVER_CONNECTION_DISPOSING`         | Finishing up; closing connection and invoking `on_response` callback.   |
| `HTTP_SERVER_CONNECTION_ERROR`             | Internal error; triggers 500 response then disposal.                    |

### 4.2 Flow Summary

```text
INITIALIZING
   ↓
READING_FIRSTLINE
   ↓
READING_HEADERS
   ↓
READING_BODY (if Content-Length)
   ↓
VALIDATING → WEATHER_HANDOVER → RESPONDING → DISPOSING
                       ↑
                     (Weather instance completes)
```

### 4.3 Function Return Summaries

- `int http_server_connection_init(HTTP_Server_Connection* conn, int fd)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, `ERR_FATAL`

- `int http_server_connection_init_ptr(int fd, HTTP_Server_Connection** conn_ptr)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, plus any from `http_server_connection_init`

- `void http_server_connection_set_callback(HTTP_Server_Connection* conn, void* ctx, http_server_connection_on_request on_req, http_server_connection_on_response on_resp)`  
  
- `void http_server_connection_taskwork(void* context, uint64_t montime)`  
  
- `HTTPServerConnectionState worktask_init(HTTP_Server_Connection* conn)`  
  - **Returns:** next connection state.

- `HTTPServerConnectionState worktask_request_read_firstline(HTTP_Server_Connection* conn)`  
  - **Returns:** next connection state.

- `HTTPServerConnectionState worktask_request_read_headers(HTTP_Server_Connection* conn)`  
  - **Returns:** next connection state.

- `HTTPServerConnectionState worktask_request_read_body(HTTP_Server_Connection* conn)`  
  - **Returns:** next connection state.

- `HTTPServerConnectionState worktask_request_validate(HTTP_Server_Connection* conn)`  
  - **Returns:** next connection state.

- `HTTPServerConnectionState worktask_respond(HTTP_Server_Connection* conn)`  
  - **Returns:** `HTTP_SERVER_CONNECTION_DISPOSING` or `HTTP_SERVER_CONNECTION_ERROR`

- `void http_server_connection_dispose(HTTP_Server_Connection* conn)`  
  
- `void http_server_connection_dispose_ptr(HTTP_Server_Connection** conn_ptr)`  

---

## 5. HTTP Parser

Parses HTTP request first line, headers, and provides helper mappings.

### 5.1 Function Return Summaries

- `int http_parser_first_line(const char* line, size_t line_len, HTTP_Request* req, Linked_List** params_out)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_BAD_FORMAT`, `ERR_NO_MEMORY`

- `int http_parser_find_line_end(const uint8_t* buf, size_t len)`  
  - **Success:** index `>= 0`  
  - **Failure:** `-1` (not found)

- `int http_parser_find_headers_end(const uint8_t* buf, size_t len)`  
  - **Success:** index `>= 0`  
  - **Failure:** `-1` (not found)

- `int http_parser_headers(const char* buf, size_t len, Linked_List** headers_out)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, `ERR_BAD_FORMAT`

- `HTTPMethod http_method_string_to_enum(const char* method_str)`  
  - **Success:** valid `HTTPMethod` value  
  - **Failure:** `HTTP_INVALID`

- `const char* http_method_enum_to_string(HTTPMethod method)`  
  - **Success:** method string  
  - **Failure:** `NULL` if invalid

- `int http_parser_get_header_value(Linked_List* headers, char* name, const char** out_value)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NOT_FOUND`

- `void http_parser_dispose_linked_list(Linked_List* list)`  

---

## 6. Weather Server

The Weather server owns the HTTP server and manages `Weather_Server_Instance` objects.

### 6.1 States (`WeatherServerState`)

| State                      | Meaning                                                        |
|----------------------------|----------------------------------------------------------------|
| `WEATHER_SERVER_INIT`      | Weather server created; HTTP init in progress.                |
| `WEATHER_SERVER_IDLE`      | Ready; waiting for new HTTP connections.                      |
| `WEATHER_SERVER_CONNECTING`| An `HTTP_Server_Connection` was signaled; doing handover.     |
| `WEATHER_SERVER_CONNECTED` | A `Weather_Server_Instance` has been created.                 |
| `WEATHER_SERVER_ERROR`     | Fatal error; will proceed to disposal.                        |
| `WEATHER_SERVER_DISPOSING` | Shutting down and cleaning up resources.                      |

### 6.2 Function Return Summaries

- `int weather_server_init(Weather_Server* server)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, `ERR_FATAL`, plus any from `http_server_init`

- `int weather_server_init_ptr(Weather_Server** server_ptr)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, plus any from `weather_server_init`

- `int weather_server_on_http_connection(void* context, HTTP_Server_Connection* connection)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`

- `WeatherServerState weather_server_connection_handover(Weather_Server* server)`  
  - **Returns:** `WEATHER_SERVER_CONNECTED` or `WEATHER_SERVER_ERROR`

- `int weather_server_on_instance_finish(void* context, void* instance)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`

- `int weather_server_on_http_error(void* context)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`

- `void weather_server_taskwork(void* context, uint64_t montime)`  
  
- `void weather_server_dispose(Weather_Server* server)`  

---

## 7. Weather Server Instances

Each `Weather_Server_Instance` handles one HTTP request/response pair at the weather domain level.

### 7.1 Instance States (`WeatherServerInstanceState`)

| State                                   | Meaning                                                    |
|-----------------------------------------|------------------------------------------------------------|
| `WEATHER_SERVER_INSTANCE_INITIALIZING`  | Instance created; task about to start.                    |
| `WEATHER_SERVER_INSTANCE_REQUEST_PARSING` | Interpreting HTTP request via Weather_API.            |
| `WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING`| Building JSON + HTTP framing.                         |
| `WEATHER_SERVER_INSTANCE_RESPONSE_SENDING` | Marking HTTP connection ready to respond.            |
| `WEATHER_SERVER_INSTANCE_DISPOSING`       | Destroying its own scheduler task.                    |
| `WEATHER_SERVER_INSTANCE_ERROR`           | Error path; triggers disposal.                         |

### 7.2 Function Return Summaries

- `int weather_server_instance_init(void* context, Weather_Server_Instance* instance, HTTP_Server_Connection* connection)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`

- `int weather_server_instance_init_ptr(void* context, HTTP_Server_Connection* connection, Weather_Server_Instance** instance_ptr)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, plus any from `weather_server_instance_init`

- `int weather_server_instance_on_request(void* context)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`

- `int weather_server_instance_on_response(void* context)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`

- `WeatherServerInstanceState worktask_request_parse(Weather_Server_Instance* instance)`  
  - **Returns:** next instance state

- `WeatherServerInstanceState worktask_response_build(Weather_Server_Instance* instance)`  
  - **Returns:** next instance state

- `void weather_server_instance_taskwork(void* context, uint64_t montime)`  
  
- `void weather_server_instance_dispose(Weather_Server_Instance* instance)`  
  
- `void weather_server_instance_dispose_ptr(Weather_Server_Instance** instance_ptr)`  

---

## 8. Weather API

Maps HTTP request paths and query parameters to weather operations.

### 8.1 Endpoints

```c
const Weather_API_Endpoint Endpoints[ENDPOINT_INVALID] = {
  { "/weather",       HTTP_GET,    ENDPOINT_WEATHER_GET      },
  { "/forecast",      HTTP_GET,    ENDPOINT_FORECAST_GET     },
  { "/cities",        HTTP_GET,    ENDPOINT_CITIES_LIST      },
  { "/cities/geo",    HTTP_GET,    ENDPOINT_CITIES_COORDS    },
  { "/cities/add",    HTTP_POST,   ENDPOINT_CITIES_ADD       },
  { "/cities/remove", HTTP_DELETE, ENDPOINT_CITIES_REMOVE    },
};
```

Currently, only `ENDPOINT_WEATHER_GET` is fully implemented; others set HTTP status 404.

### 8.2 Function Return Summaries

- `int weather_api_init_ptr(Weather_API** api_ptr, HTTP_Request* req, HTTP_Response* res)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`

- `WeatherAPIEndpoint weather_api_get_endpoint(const char* request_path)`  
  - **Success:** valid endpoint enum  
  - **Failure:** `ENDPOINT_INVALID`

- `int weather_api_handle_endpoint(Weather_API* api)`  
  - **Success:** `SUCCESS`

- `int weather_api_handle_endpoint_weather_get(Weather_API* api)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INVALID_ARG`, `ERR_NO_MEMORY`, `ERR_IO`, `ERR_PARSE`

- `void weather_api_dispose_ptr(Weather_API** api_ptr)`  

---

## 9. Meteo API

Handles external Open-Meteo calls and parsing into `Meteo_Weather`.

### 9.1 Function Return Summaries

- `int meteo_init_ptr(Meteo_Weather** mw_ptr)`  
  - **Success:** `0`  
  - **Failure:** negative error code (e.g., `-2` for allocation failure)

- `int meteo_get_weather(Meteo_Weather* mw, float lat, float lon, bool forecast)`  
  - **Success:** `0`  
  - **Failure:** negative error code

- `const char* meteo_get_weather_json(float lat, float lon, bool forecast)`  
  - **Success:** heap-allocated JSON string pointer  
  - **Failure:** `NULL`

- `int meteo_parse_json(Meteo_Weather* mw, const char* json)`  
  - **Success:** `0`  
  - **Failure:** negative error code (parse/allocation failures)

- `void meteo_dispose_ptr(Meteo_Weather** mw_ptr)`  

---

## 10. Weather Parser

Translates `Meteo_Weather` into `Weather`/`City` and builds JSON responses.

### 10.1 Function Return Summaries

- `int weather_parser_init_ptr(City** c_ptr, Weather** w_ptr, Forecast** f_ptr)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_NO_MEMORY`

- `int weather_parser_get_weather_meteo(City* city, bool forecast)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_NO_MEMORY`, `ERR_IO`, `ERR_PARSE`

- `int weather_parser_parse_weather_meteo(Weather* weather, Meteo_Weather* mw)`  
  - **Success:** `SUCCESS`  
  - **Failure:** `ERR_INTERNAL`

- `char* weather_parser_build_weather_json(Weather* weather)`  
  - **Success:** heap-allocated JSON string pointer (or `""` if `weather` is `NULL`)

- `int weather_parser_lat_lon(const char* val, float* target_coord)`  
  - **Success:** `1` (parsed) or `0` (nothing parsed)  
  - **Failure:** `-1` (error)

- `void weather_parser_dispose_ptr(City** c_ptr, Weather** w_ptr, Forecast** f_ptr)`  

---

## 11. Scheduler

Global cooperative scheduler that drives all tasks.

### 11.1 Function Return Summaries

- `int scheduler_init()`  
  - **Success:** `0`

- `Scheduler_Task* scheduler_create_task(void* context, void (*callback)(void* context, uint64_t montime))`  
  - **Success:** pointer to `Scheduler_Task`  
  - **Failure:** `NULL` (no free slot)

- `void scheduler_destroy_task(Scheduler_Task* task)`  
  
- `void scheduler_work(uint64_t montime)`  
  
- `int scheduler_get_task_count()`  
  - **Success:** number of active tasks

- `void scheduler_dispose()`  

---

## 12. Main Program (Weather Server Entry Point)

The `main` function:

1. Initializes scheduler and Weather server.  
2. Optionally initializes global CURL.  
3. Installs `SIGINT` handler for graceful shutdown.  
4. Runs:

```c
while (!stop) {
  scheduler_work(SystemMonotonicMS());
}
```

5. Cleans up on exit and returns 0.

---

## 13. Cross-Component Sequence (End-to-End)

```text
TCP_SERVER_LISTENING
   │
   ├─ accept(fd) → SUCCESS
   ↓
HTTP_SERVER_CONNECTING (http_server_on_accept)
   ↓
HTTP_Server_Connection created
   ↓
Weather_Server receives connection
   ↓
Weather_Server_Instance created
   ↓
HTTP Connection parses request
   ↓
Weather Instance builds JSON + full HTTP response
   ↓
HTTP Connection sends response
   ↓
HTTP Connection disposes and calls Weather instance on_response
   ↓
Weather instance removed from list and freed
```


