#include "../../include/weather.h"

/* -----------------Internal Functions----------------- */

void weather_server_taskwork(void* _context, uint64_t _montime);
int weather_server_on_http_connection(void* _context, HTTP_Server_Connection* _Connection);

/* ---------------------------------------------------- */

int weather_server_init(Weather_Server* _Server)
{

  if (!_Server)
    return -1;
  /*_Server->http_server = NULL;*/
  _Server->instances = NULL;
  _Server->task = NULL;
  _Server->state = WEATHER_SERVER_INIT;

  int result;

  result = http_server_init(&_Server->http_server, weather_server_on_http_connection, _Server);

  if (result != 0){

    _Server->state = WEATHER_SERVER_ERROR;

    return -1;
  }

  Linked_List* Instances = linked_list_create();
  _Server->instances = Instances;

  _Server->task = scheduler_create_task(_Server, weather_server_taskwork);
  _Server->state = WEATHER_SERVER_IDLE;

  return 0;
}

int weather_server_init_ptr(Weather_Server** _Server_Ptr)
{
  int result;

  _Server_Ptr = malloc(sizeof(Weather_Server));
  if (_Server_Ptr == NULL)
    return -1;

  result = weather_server_init(*_Server_Ptr);
  if (result != 0)
    return -2;

  return 0;
}

/* --------------TASKWORK STATE FUNCTIONS-------------- */
WeatherServerState weather_server_handle_request(Weather_Server* _Server)
{

  return WEATHER_SERVER_IDLE; 
}
/*
WEATHER_SERVER_INITING 
WEATHER_SERVER_RUNNING,
WEATHER_SERVER_HANDLING_REQUEST,
WEATHER_SERVER_DONE
*/
/* ---------------------------------------------------- */

int weather_server_on_http_connection(void* _context, HTTP_Server_Connection* _Connection)
{

  Weather_Server* _Server = (Weather_Server*)_context;

  Weather_Server_Instance* Instance = NULL;
  int result = weather_server_instance_init_ptr(_Connection, &Instance);
  if(result != 0)
  {
    printf("WeatherServer_OnHTTPConnection: Failed to initiate instance\n");
    return -1;
  }

  Linked_Item* LI;

  linked_list_item_add(_Server->instances, &LI, Instance);

  return 0;
}
int weather_server_on_http_error(void* _context)
{
  if (!_context)
  return -1;

  Weather_Server* server = (Weather_Server*)_context;
  server->state = WEATHER_SERVER_DISPOSE;
  return 0;

}

void weather_server_taskwork(void* _Context, uint64_t _MonTime)
{
	Weather_Server* _Server = (Weather_Server*)_Context;

  Weather_Server_Instance* Instance;
  weather_server_instance_taskwork(Instance, _MonTime);
}

void weather_server_dispose(Weather_Server* _Server)
{
  linked_list_destroy(&_Server->instances);
	http_server_dispose(&_Server->http_server);
	scheduler_destroy_task(_Server->task);
}

