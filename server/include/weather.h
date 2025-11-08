#ifndef __WEATHER_H__
#define __WEATHER_H__

/* ******************************************************************* */
/* ******************** WEATHER SERVER INSTANCE ********************** */
/* ******************************************************************* */

#include "http.h"

typedef enum
{
  WEATHER_SERVER_INSTANCE_INITING,
  WEATHER_SERVER_INSTANCE_PARSING_REQUEST,
  WEATHER_SERVER_INSTANCE_BUILDING_RESPONSE,
  WEATHER_SERVER_INSTANCE_DONE

} WeatherServerInstanceState;

typedef struct
{
  HTTP_Server_Connection* http_connection;

} Weather_Server_Instance;


int weather_server_instance_init(Weather_Server_Instance* _Server, HTTP_Server_Connection* _Connection);
int weather_server_instance_init_ptr(HTTP_Server_Connection* _Connection, Weather_Server_Instance** _Server_Ptr);

void weather_server_instance_taskwork(Weather_Server_Instance* _Instance, uint64_t _montime);

void weather_server_instance_dispose(Weather_Server_Instance* _Server);
void weather_server_instance_dispose_ptr(Weather_Server_Instance** _Server_Ptr);

/* ******************************************************************* */
/* ************************* WEATHER SERVER ************************** */
/* ******************************************************************* */

#include "../../utils/include/linked_list.h"
#include "scheduler.h"

typedef enum
{
  WEATHER_SERVER_INITING,
  WEATHER_SERVER_RUNNING,
  WEATHER_SERVER_HANDLING_REQUEST,
  WEATHER_SERVER_DONE

/*
  WEATHER_SERVER_STATE_INITIALIZING
  WEATHER_SERVER_STATE_LISTENING
  WEATHER_SERVER_STATE_CONNECTING
  WEATHER_SERVER_STATE_CONNECTED
  WEATHER_SERVER_STATE_ERROR
  WEATHER_SERVER_STATE_DISPOSING
*/


} WeatherServerState;

typedef struct
{
	HTTP_Server     http_server;
	Scheduler_Task* task;
  Linked_List*    instances;

} Weather_Server;


int weather_server_init(Weather_Server* _Server);
int weather_server_init_ptr(Weather_Server** _Server_Ptr);

void weather_server_dispose(Weather_Server* _Server);
void weather_server_dispose_ptr(Weather_Server** _Server_Ptr);

#endif
