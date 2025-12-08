#include "weather/weather_server.h"

/* -----------------Internal Functions----------------- */

void weather_server_taskwork(void* _context, uint64_t _montime);
int weather_server_on_http_connection(void* _context, HTTP_Server_Connection* _Connection);
int weather_server_on_instance_finish(void* _context, void* _instance);

WeatherServerState weather_server_connection_handover(Weather_Server* _Server);
/* ---------------------------------------------------- */

int weather_server_init(Weather_Server* _Server)
{

  if (!_Server) {
    return ERR_INVALID_ARG;
  }

  /*_Server->http_server = NULL;*/
  _Server->instances = NULL;
  _Server->task = NULL;
  _Server->state = WEATHER_SERVER_INIT;
  _Server->http_connection = NULL;

  int result = http_server_init(&_Server->http_server, weather_server_on_http_connection, _Server);
  if (result != SUCCESS){

    _Server->state = WEATHER_SERVER_ERROR;

    return result;
  }

  Linked_List* Instances = linked_list_create();

  if (!Instances) {
    http_server_dispose(&_Server->http_server);
    _Server->state = WEATHER_SERVER_ERROR;
    return ERR_NO_MEMORY;
  }

  _Server->instances = Instances;
  _Server->task = scheduler_create_task(_Server, weather_server_taskwork);
  if (!_Server->task) {
    linked_list_destroy(&_Server->instances);
    http_server_dispose(&_Server->http_server);
    _Server->state = WEATHER_SERVER_ERROR;
    return ERR_FATAL;
  }

  _Server->state = WEATHER_SERVER_IDLE;

  create_directory_if_not_exists(DATA_DIR);
  create_directory_if_not_exists(CACHE_DIR);

  return SUCCESS;
}

int weather_server_init_ptr(Weather_Server** _Server_Ptr)
{

  if (!_Server_Ptr) {
    return ERR_INVALID_ARG;
  }

  Weather_Server* Server = calloc(1, sizeof(Weather_Server));
  if (!Server) {
    return ERR_NO_MEMORY;
  }

  int result = weather_server_init(Server);
  if (result != SUCCESS) {
    free(Server);
    return result;
  }

  *_Server_Ptr = Server;

  return SUCCESS;
}

/* --------------TASKWORK STATE FUNCTIONS-------------- */

/* ---------------------------------------------------- */

int weather_server_on_http_connection(void* _context, HTTP_Server_Connection* _Connection)
{
  if (!_context || ! _Connection) {
    return ERR_INVALID_ARG;
  }

  Weather_Server* Server = (Weather_Server*)_context;
  Server->http_connection = _Connection;

  Server->state = WEATHER_SERVER_CONNECTING;
  return SUCCESS;

}

WeatherServerState weather_server_connection_handover(Weather_Server* _Server)
{
  if (!_Server) {
    return WEATHER_SERVER_ERROR;
  }

  Weather_Server_Instance* Instance = NULL;
  int result = weather_server_instance_init_ptr(_Server, _Server->http_connection, &Instance);
  if(result != SUCCESS)
  {
    return WEATHER_SERVER_ERROR;
  }

  Linked_Item* LI;
  linked_list_item_add(_Server->instances, &LI, Instance);
  Instance->item = LI;

  Instance->on_finish = weather_server_on_instance_finish;

  _Server->http_connection = NULL;
  return WEATHER_SERVER_CONNECTED;
}

int weather_server_on_instance_finish(void* _context, void* _instance)
{
  if (!_context || !_instance) {
    return ERR_INVALID_ARG;
  }

  Weather_Server* Server = (Weather_Server*)_context;
  Weather_Server_Instance* Instance = (Weather_Server_Instance*)_instance;
  if (Instance->item != NULL)
    linked_list_item_remove(Server->instances, Instance->item);

  linked_list_foreach(Server->instances, node)
    printf("Instance: %p\n", node);

  weather_server_instance_dispose_ptr(&Instance);

  return SUCCESS;
}

int weather_server_on_http_error(void* _context)
{
  if (!_context) {
    return ERR_INVALID_ARG;
  }

  Weather_Server* server = (Weather_Server*)_context;
  server->state = WEATHER_SERVER_DISPOSING;

  return SUCCESS;
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
      weather_server_dispose(server);
      /*CALL DISPOSE STUFF HERE*/
      printf("WEATHER_SERVER_DISPOSE\n");
      break;
  }

  server->state = next_state;

}

void weather_server_dispose(Weather_Server* _Server)
{
  if (!_Server) {
    return;
  }

  if (_Server->instances) {
    linked_list_destroy(&_Server->instances);
  }
	http_server_dispose(&_Server->http_server);

  if (_Server->task) {
    scheduler_destroy_task(_Server->task);
  }
}

