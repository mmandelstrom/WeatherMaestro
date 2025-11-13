#include "../../include/weather.h"

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
  _Instance->context = _context; // Weather_Server
  _Instance->task = NULL; // Weather_Server
  _Instance->http_connection = _Connection;
  http_server_connection_set_callback(_Instance->http_connection, _Instance, weather_server_instance_on_request, weather_server_instance_on_response);

  return 0;
}
int weather_server_instance_init_ptr(void* _context, HTTP_Server_Connection* _Connection, Weather_Server_Instance** _Instance_Ptr)
{

	if(_Instance_Ptr == NULL)
		return -1;

	Weather_Server_Instance* _Instance = (Weather_Server_Instance*)malloc(sizeof(Weather_Server_Instance));
	if(_Instance == NULL)
		return -2;

	int result = weather_server_instance_init(_context, _Instance, _Connection);
	if(result != 0)
	{
		free(_Instance);
		return result;
	}

	*(_Instance_Ptr) = _Instance;

	return 0;
}

int weather_server_instance_on_request(void* _context)
{
  Weather_Server_Instance* _Instance = (Weather_Server_Instance*)_context;
  _Instance->task = scheduler_create_task(_Instance, weather_server_instance_taskwork);

  return 0;
}

int weather_server_instance_on_response(void* _context)
{
  Weather_Server_Instance* _Instance = (Weather_Server_Instance*)_context;
  if (_Instance->task != NULL)
    scheduler_destroy_task(_Instance->task);

  _Instance->on_finish(_Instance->context, _Instance);
     
  return 0;
}


WeatherServerInstanceState worktask_request_parse(Weather_Server_Instance* _Instance)
{
  /* This will take the http parsed Request and find out what the user wants 
   * Should populate necessery data structs so that an appropriate response can be built*/
  HTTP_Request* Request = &_Instance->http_connection->request;

  printf("Method: %i\r\n", Request->method);


  return WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING;
}
WeatherServerInstanceState worktask_response_build(Weather_Server_Instance* _Instance)
{
  HTTP_Response* Response = &_Instance->http_connection->response;

  return WEATHER_SERVER_INSTANCE_RESPONSE_SENDING;
}

void weather_server_instance_taskwork(void* _context, uint64_t _montime)
{
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
      printf("WEATHER_SERVER_INSTANCE_DISPOSING\n");

      //wait for connection to finish
      if (_Instance->http_connection->state != HTTP_SERVER_CONNECTION_RESPONDING)
      {
        _Instance->on_finish(_Instance->context, _Instance);
        scheduler_destroy_task(_Instance->task);
      }

    } break;
  }
}

void weather_server_instance_dispose(Weather_Server_Instance* _Instance)
{
  if (_Instance->http_connection != NULL) // Should http_server take responsibility for this instead?
    http_server_connection_dispose_ptr(&_Instance->http_connection);

  _Instance->http_connection = NULL;
}
void weather_server_instance_dispose_ptr(Weather_Server_Instance** _Instance_Ptr)
{
  weather_server_instance_dispose(*_Instance_Ptr);

  free(*_Instance_Ptr);
  _Instance_Ptr = NULL;

}

