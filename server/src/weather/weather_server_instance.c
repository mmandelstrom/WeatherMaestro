#include "../../include/weather.h"

void weather_server_instance_taskwork(void* _Context, uint64_t _MonTime);
int weather_server_instance_on_http_connection(void* _Context, HTTP_Server_Connection* _Connection);

int weather_server_instance_initiate(WeatherServer* _Server)
{
  int result;

  result = http_server_init(&_Server->http_server, weather_server_instance_on_http_connection);
  if (result != 0)
    return result;

  _Server->task = scheduler_create_task(_Server, weather_server_instance_taskwork);

  return 0;
}
int weather_server_instance_initiate_ptr(WeatherServer** _ServerPtr)
{

  return 0;
}


void weather_server_instance_taskwork(void* _Context, uint64_t _MonTime)
{
  
}


void weather_server_instance_dispose(WeatherServer* _Server)
{
	http_server_dispose(&_Server->http_server);
	scheduler_destroy_task(_Server->task);
}
void weather_server_instance_dispose_ptr(WeatherServer** _ServerPtr)
{

}



