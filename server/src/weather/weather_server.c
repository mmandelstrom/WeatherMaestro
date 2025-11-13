#include "../../include/weather.h"
#include <asm-generic/errno-base.h>

/* -----------------Internal Functions----------------- */

void weather_server_taskwork(void* _context, uint64_t _montime);
int weather_server_on_http_connection(void* _context, HTTP_Server_Connection* _Connection);
WeatherServerState weather_server_connection_handover(Weather_Server* _Server);
/* ---------------------------------------------------- */

int weather_server_init(Weather_Server* _Server)
{

  if (!_Server)
    return -1;
  /*_Server->http_server = NULL;*/
  _Server->instances = NULL;
  _Server->task = NULL;
  _Server->state = WEATHER_SERVER_INIT;
  _Server->http_connection = NULL;

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

/* ---------------------------------------------------- */
int weather_server_on_http_connection(void* _context, HTTP_Server_Connection* _Connection)
{
  if (!_context) {
    errno = EINVAL;
    return -1;
  }  

  Weather_Server* server = (Weather_Server*)_context;
  server->http_connection = _Connection;

  server->state = WEATHER_SERVER_CONNECTING;
  return 0;

}

WeatherServerState weather_server_connection_handover(Weather_Server* _Server)
{
  if (!_Server) {
    errno = EINVAL;
    return WEATHER_SERVER_ERROR;
  }
/*
  Weather_Server_Instance* Instance = NULL;
  int result = weather_server_instance_init_ptr(_Connection, &Instance);
  if(result != 0)
  {
    printf("WeatherServer_OnHTTPConnection: Failed to initiate instance\n");
    return -1;
  }

  Linked_Item* LI;

  linked_list_item_add(_Server->instances, &LI, Instance);
  */

  _Server->http_connection = NULL;
  return WEATHER_SERVER_CONNECTED;
}


int weather_server_on_http_error(void* _context)
{
  if (!_context)
    return -1;

  Weather_Server* server = (Weather_Server*)_context;
  server->state = WEATHER_SERVER_DISPOSING;
  return 0;

}

void weather_server_taskwork(void* _context, uint64_t _MonTime)
{
  if (!_context) 
    return;

  Weather_Server* server = (Weather_Server*)_context;
  
  WeatherServerState next_state = server->state;

  switch(server->state) {
    case WEATHER_SERVER_INIT:
      printf("WEATHER_SERVER_INIT\n");
      next_state = WEATHER_SERVER_IDLE;
      break;

    case WEATHER_SERVER_IDLE: {
        break;
      }
      
    case WEATHER_SERVER_CONNECTING: {
      next_state = weather_server_connection_handover(server);
      break;
      }

    case WEATHER_SERVER_CONNECTED:
      printf("WEATHER_SERVER_CONNECTED\n");
      next_state = WEATHER_SERVER_IDLE;
      break;

    case WEATHER_SERVER_ERROR:
      printf("WEATHER_SERVER_ERROR\n");
      next_state = WEATHER_SERVER_DISPOSING;
      break;

    case WEATHER_SERVER_DISPOSING:
      /*CALL DISPOSE STUFF HERE*/
      printf("WEATHER_SERVER_DISPOSE\n");
      break;
  }

  server->state = next_state;

}

void weather_server_dispose(Weather_Server* _Server)
{
  linked_list_destroy(&_Server->instances);
	http_server_dispose(&_Server->http_server);
	scheduler_destroy_task(_Server->task);
}

