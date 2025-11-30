#include "../../include/weather/weather_instance.h"

//-----------------Internal Functions-----------------
//
int weather_server_instance_on_http_connection(void* _context, HTTP_Server_Connection* _Connection);
int weather_server_instance_on_request(void* _context);
int weather_server_instance_on_response(void* _context);

void weather_server_instance_taskwork(void* _context, uint64_t _montime);
WeatherServerInstanceState worktask_request_parse(Weather_Server_Instance* _Instance);
WeatherServerInstanceState worktask_response_build(Weather_Server_Instance* _Instance);
//----------------------------------------------------

int weather_server_instance_init(void* _context, Weather_Server_Instance* _Instance, HTTP_Server_Connection* _Connection)
{
  if (!_Instance || !_Connection) {
    return ERR_INVALID_ARG;
  }

  memset(_Instance, 0, sizeof(Weather_Server_Instance));

  _Instance->context = _context;
  _Instance->task = NULL;
  _Instance->http_connection = _Connection;
  http_server_connection_set_callback(_Instance->http_connection, _Instance, weather_server_instance_on_request, weather_server_instance_on_response);

  return SUCCESS;
}
int weather_server_instance_init_ptr(void* _context, HTTP_Server_Connection* _Connection, Weather_Server_Instance** _Instance_Ptr)
{

	if(_Instance_Ptr == NULL)
		return ERR_INVALID_ARG;

	Weather_Server_Instance* _Instance = (Weather_Server_Instance*)malloc(sizeof(Weather_Server_Instance));
	if(_Instance == NULL)
		return ERR_NO_MEMORY;

	int result = weather_server_instance_init(_context, _Instance, _Connection);
	if(result != SUCCESS)
	{
		free(_Instance);
		return result;
	}

	*(_Instance_Ptr) = _Instance;

	return SUCCESS;
}

int weather_server_instance_on_request(void* _context)
{
  if (!_context) {
    return ERR_INVALID_ARG;
  }

  Weather_Server_Instance* _Instance = (Weather_Server_Instance*)_context;
  _Instance->state = WEATHER_SERVER_INSTANCE_INITIALIZING;
  _Instance->task = scheduler_create_task(_Instance, weather_server_instance_taskwork);

  return SUCCESS;
}

int weather_server_instance_on_response(void* _context)
{
  if (!_context) {
    return ERR_INVALID_ARG;
  }

  Weather_Server_Instance* _Instance = (Weather_Server_Instance*)_context;
  if (_Instance->task != NULL)
    scheduler_destroy_task(_Instance->task);

  _Instance->on_finish(_Instance->context, _Instance);
     
  return SUCCESS;
}


WeatherServerInstanceState worktask_request_parse(Weather_Server_Instance* _Instance)
{
  if (!_Instance) {
    return WEATHER_SERVER_INSTANCE_ERROR;
  }

  if (weather_api_init_ptr(&_Instance->weather_api, _Instance->http_connection->request, _Instance->http_connection->response) != 0 || weather_api_handle_endpoint(_Instance->weather_api) != 0)
  {
    _Instance->http_connection->response->status_code = 500;
    return WEATHER_SERVER_INSTANCE_RESPONSE_SENDING;
  }


  return WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING;
}

WeatherServerInstanceState worktask_response_build(Weather_Server_Instance* _Instance)
{
  if (!_Instance) {
    return WEATHER_SERVER_INSTANCE_ERROR;
  }

  HTTP_Response* Res = _Instance->http_connection->response;
  if (Res->body != NULL && Res->status_code == 200)
  {
    size_t body_len = strlen(Res->body);

    const char* reason_phrase = HttpStatus_reasonPhrase(Res->status_code);
    char firstline[128];
    size_t firstline_len = snprintf(firstline, 128, HTTP_RESPONSE_FIRSTLINE_TEMPLATE,
                                  Res->status_code,
                                  reason_phrase);
    firstline[firstline_len] = '\0';

    char headers_buf[512];
    size_t headers_len = snprintf(headers_buf, 512,
             "Content-Length: %zu\r\n"
             "Content-Type: application/json\r\n"
             "Connection: close\r\n\r\n", body_len);

    headers_buf[headers_len] = '\0';

    size_t full_response_len = body_len + headers_len + firstline_len + 2;
    char full_response[full_response_len + 1];
    snprintf(full_response, full_response_len, "%s%s%s\r\n", firstline, headers_buf, Res->body);

    _Instance->http_connection->response->full_response = malloc(full_response_len);
    if (_Instance->http_connection->response->full_response == NULL)
    {
      // handle
      perror("malloc");
    }
    memcpy(_Instance->http_connection->response->full_response, full_response, full_response_len);
  }

  return WEATHER_SERVER_INSTANCE_RESPONSE_SENDING;
}

void weather_server_instance_taskwork(void* _context, uint64_t _montime)
{

  if(!_context) {
    return;
  }
  (void)_montime;

  Weather_Server_Instance* _Instance = (Weather_Server_Instance*)_context;

  switch (_Instance->state)
  {
    case WEATHER_SERVER_INSTANCE_INITIALIZING:
    {
      printf("WEATHER_SERVER_INSTANCE_INITIALIZING\n");
      _Instance->state = WEATHER_SERVER_INSTANCE_REQUEST_PARSING; 
    } break;

    case WEATHER_SERVER_INSTANCE_REQUEST_PARSING:
    {
      printf("WEATHER_SERVER_INSTANCE_REQUEST_PARSING\n");
      _Instance->state = worktask_request_parse(_Instance);
    } break;

    case WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING:
    {
      printf("WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING\n");
      _Instance->state = worktask_response_build(_Instance);
    } break;

    case WEATHER_SERVER_INSTANCE_RESPONSE_SENDING:
    {
      printf("WEATHER_SERVER_INSTANCE_RESPONSE_SENDING\n");

      _Instance->http_connection->weather_done = 1;
      _Instance->http_connection->state = HTTP_SERVER_CONNECTION_RESPONDING;
      
      _Instance->state = WEATHER_SERVER_INSTANCE_DISPOSING;
    } break;

    case WEATHER_SERVER_INSTANCE_DISPOSING:
    {
      printf("WEATHER_SERVER_INSTANCE_DISPOSING (%p)\n", _Instance);
      scheduler_destroy_task(_Instance->task);

    } break;

    case WEATHER_SERVER_INSTANCE_ERROR:
    {
      _Instance->state = WEATHER_SERVER_INSTANCE_DISPOSING;
      weather_server_instance_dispose(_Instance);
    } break;
  }
}

void weather_server_instance_dispose(Weather_Server_Instance* _Instance)
{
  if (_Instance->http_connection != NULL) 
    http_server_connection_dispose_ptr(&_Instance->http_connection);
  _Instance->http_connection = NULL;

  if (_Instance->weather_api != NULL)
    weather_api_dispose_ptr(&_Instance->weather_api);
  _Instance->weather_api = NULL;

}
void weather_server_instance_dispose_ptr(Weather_Server_Instance** _Instance_Ptr)
{
  weather_server_instance_dispose(*_Instance_Ptr);

  free(*_Instance_Ptr);
  _Instance_Ptr = NULL;

}

