#include "../../include/weather.h"

//-----------------Internal Functions-----------------
//
int weather_server_instance_on_http_connection(void* _context, HTTP_Server_Connection* _Connection);
int weather_server_instance_on_request(void* _context);

void weather_server_instance_taskwork(Weather_Server_Instance* _Instance, uint64_t _montime);

//----------------------------------------------------

int weather_server_instance_init(Weather_Server_Instance* _Instance, HTTP_Server_Connection* _Connection)
{
  _Instance->http_connection = _Connection;
  http_server_connection_set_callback(_Instance->http_connection, _Instance, weather_server_instance_on_request);
  /* _Instance->task = scheduler_create_task(_Server, weather_server_instance_taskwork); */

  return 0;
}
int weather_server_instance_init_ptr(HTTP_Server_Connection* _Connection, Weather_Server_Instance** _Instance_Ptr)
{

	if(_Instance_Ptr == NULL)
		return -1;

	Weather_Server_Instance* _Instance = (Weather_Server_Instance*)malloc(sizeof(Weather_Server_Instance));
	if(_Instance == NULL)
		return -2;

	int result = weather_server_instance_init(_Instance, _Connection);
	if(result != 0)
	{
		free(_Instance);
		return result;
	}

	*(_Instance_Ptr) = _Instance;

	return 0;
}

WeatherServerInstanceState validate_request(Weather_Server_Instance* _Instance)
{
  int valid = 0;

  if (valid > 0) 
    return WEATHER_SERVER_INSTANCE_REQUEST_PARSING;
  else 
    return WEATHER_SERVER_INSTANCE_RESPONSE_SEND;
}

void weather_server_instance_taskwork(Weather_Server_Instance* _Instance, uint64_t _montime)
{
  /* switch (_Instance->state)
  {
    case WEATHER_SERVER_INSTANCE_INITIALIZING:
    {
      printf("WEATHER_SERVER_INSTANCE_INITIALIZING\n");
    } break;

    case WEATHER_SERVER_INSTANCE_REQUEST_VALIDATING:
    {
      printf("WEATHER_SERVER_INSTANCE_REQUEST_VALIDATING\n");
    } break;

    case WEATHER_SERVER_INSTANCE_REQUEST_PARSING:
    {
      printf("WEATHER_SERVER_INSTANCE_REQUEST_PARSING\n");
    } break;

    case WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING:
    {
      printf("WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING\n");
    } break;

    case WEATHER_SERVER_INSTANCE_RESPONSE_SEND:
    {
      printf("WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING\n");
    } break;

    case WEATHER_SERVER_INSTANCE_DISPOSING:
    {
      printf("WEATHER_SERVER_INSTANCE_DISPOSING\n");
    } break;
  } */
}

int weather_server_instance_on_request(void* _Context)
{
	Weather_Server_Instance* _Server = (Weather_Server_Instance*)_Context;
  HTTP_Request Req = _Server->http_connection->request;

	printf("Method: %i\r\n", _Server->http_connection->request.method);

  _Server->http_connection->weather_done = 1;

	return 0;
}

void weather_server_instance_dispose(Weather_Server_Instance* _Instance)
{
  if (_Instance->http_connection != NULL)
    http_server_connection_dispose(_Instance->http_connection);
	/* http_server_dispose(&_Server->http_server); */
	/* scheduler_destroy_task(_Instance->task); */
}
void weather_server_instance_dispose_ptr(Weather_Server_Instance** _Instance_Ptr)
{
  weather_server_instance_dispose(*_Instance_Ptr);

  free(*_Instance_Ptr);
  _Instance_Ptr = NULL;

}



