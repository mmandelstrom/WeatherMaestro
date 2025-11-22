#ifndef __WEATHER_INSTANCE_H__
#define __WEATHER_INSTANCE_H__

/* ******************************************************************* */
/* ******************** WEATHER SERVER INSTANCE ********************** */
/* ******************************************************************* */

#include "../http/http_connection.h"
#include "../api/weather_api.h"

typedef int (*weather_server_instance_on_finish)(void* _context, void* _instance);

typedef enum
{
  WEATHER_SERVER_INSTANCE_INITIALIZING,
  WEATHER_SERVER_INSTANCE_REQUEST_PARSING,
  WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING,
  WEATHER_SERVER_INSTANCE_RESPONSE_SENDING,
  WEATHER_SERVER_INSTANCE_DISPOSING

} WeatherServerInstanceState;

typedef struct
{
  WeatherServerInstanceState          state;
  Scheduler_Task*                     task;

  Linked_Item*                        item;

  HTTP_Server_Connection*             http_connection;

  void*                               context; // Weather_Server
  weather_server_instance_on_finish   on_finish;

  Weather_Request*                    weather_request;

} Weather_Server_Instance;


int weather_server_instance_init(void* _context, Weather_Server_Instance* _Instance, HTTP_Server_Connection* _Connection);
int weather_server_instance_init_ptr(void* _context, HTTP_Server_Connection* _Connection, Weather_Server_Instance** _Instance_Ptr);

void weather_server_instance_dispose(Weather_Server_Instance* _Instance);
void weather_server_instance_dispose_ptr(Weather_Server_Instance** _Instance_Ptr);


#endif
