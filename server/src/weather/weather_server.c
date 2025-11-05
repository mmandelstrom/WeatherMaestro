#include "../../include/weather.h"

void weather_server_taskwork(void* _context, uint64_t _montime);
int weather_server_on_http_connection(void* _context, HTTP_Server_Connection* _Connection);

int weather_server_init(WeatherServer* _Server)
{
  int result;

  result = http_server_init(&_Server->http_server, weather_server_on_http_connection);

  if (result != 0)
    return -1;

  _Server->task = scheduler_create_task(_Server, weather_server_taskwork);

  return 0;
}

int weather_server_init_ptr(WeatherServer** _Server_Ptr)
{
  int result;

  _Server_Ptr = malloc(sizeof(WeatherServer));
  if (_Server_Ptr == NULL)
    return -1;

  result = weather_server_init(*_Server_Ptr);
  if (result != 0)
    return result;

  return 0;
}

// --- TASKWORK STATE FUNCTIONS ---
WeatherServerState weather_server_handle_request(Weather_Server* _Server)
{

  return WEATHER_SERVER_RUNNING; 
}
/*
WEATHER_SERVER_INITING 
WEATHER_SERVER_RUNNING,
WEATHER_SERVER_HANDLING_REQUEST,
WEATHER_SERVER_DONE
*/

void weather_server_taskwork(void* _Context, uint64_t _MonTime)
{
  
}

int weather_server_on_http_connection(void* _Context, HTTP_Server_Connection* _Connection)
{
  
}

void weather_server_dispose(WeatherServer* _Server)
{
	http_server_dispose(&_Server->http_server);
	scheduler_destroy_task(_Server->task);
}

