#ifndef __WEATHER_H__
#define __WEATHER_H__

/* ******************************************************************* */
/* **************************** WEATHER ****************************** */
/* ******************************************************************* */

#include <time.h>

typedef struct
{
  time_t      timestamp;
  int         weather_code; // Maybe meteo specific this one? 

  double      temperature; 
  const char* temperature_unit; 

  double      windspeed; 
  const char* windspeed_unit;

  int         winddirection; 
  const char* winddirection_unit;

  double      precipitation;
  const char* precipitation_unit;

} Weather;

typedef struct
{
  int         count;
  Weather*    weather;

} Forecast;

/* ******************************************************************* */
/* ******************** WEATHER SERVER INSTANCE ********************** */
/* ******************************************************************* */

#include "http.h"

/* typedef int (*weather_server_instance_on_finish)(void* _context); */

typedef enum
{
  WEATHER_SERVER_INSTANCE_INITIALIZING,
  WEATHER_SERVER_INSTANCE_REQUEST_VALIDATING,
  WEATHER_SERVER_INSTANCE_REQUEST_PARSING,
  WEATHER_SERVER_INSTANCE_RESPONSE_BUILDING,
  WEATHER_SERVER_INSTANCE_RESPONSE_SEND,
  WEATHER_SERVER_INSTANCE_DISPOSING

} WeatherServerInstanceState;

typedef struct
{
  WeatherServerInstanceState          state;

  /* void*                               context;
  weather_server_instance_callback    on_request; */

  Scheduler_Task*                     task;
  HTTP_Server_Connection*             http_connection;

} Weather_Server_Instance;


int weather_server_instance_init(Weather_Server_Instance* _Server, HTTP_Server_Connection* _Connection);
int weather_server_instance_init_ptr(HTTP_Server_Connection* _Connection, Weather_Server_Instance** _Server_Ptr);


void weather_server_instance_dispose(Weather_Server_Instance* _Server);
void weather_server_instance_dispose_ptr(Weather_Server_Instance** _Server_Ptr);

/* ******************************************************************* */
/* ************************* WEATHER SERVER ************************** */
/* ******************************************************************* */

#include "../../utils/include/linked_list.h"
#include "scheduler.h"

typedef enum
{
  WEATHER_SERVER_INIT,
  WEATHER_SERVER_IDLE,
  WEATHER_SERVER_HANDOVER,
  WEATHER_SERVER_ERROR,
  WEATHER_SERVER_DISPOSE


} WeatherServerState;

typedef enum
{
  WEATHER_SERVER_ERROR_NONE = 0,
  WEATHER_SERVER_ERROR_INIT_FAILED,
  
} WeatherServerErrorState;

typedef struct
{
	HTTP_Server     http_server;
	Scheduler_Task* task;
  Linked_List*    instances;
  WeatherServerState state;
  int             handover_done;
} Weather_Server;


int weather_server_init(Weather_Server* _Server);
int weather_server_init_ptr(Weather_Server** _Server_Ptr);

void weather_server_dispose(Weather_Server* _Server);
void weather_server_dispose_ptr(Weather_Server** _Server_Ptr);

#endif
