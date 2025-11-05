#ifndef __WEATHER_H__
#define __WEATHER_H__

/* ******************************************************************* */
/* ************************* WEATHER SERVER ************************** */
/* ******************************************************************* */

#include "../../utils/include/linked_list.h"
#include "http.h"
#include "scheduler.h"

typedef enum
{
  WEATHER_SERVER_INITING,
  WEATHER_SERVER_RUNNING,
  WEATHER_SERVER_HANDLING_REQUEST,
  WEATHER_SERVER_DONE

} WeatherServerState;

typedef struct
{
	HTTP_Server http_server;
	Scheduler_Task* task;

} Weather_Server;


int weather_server_init(Weather_Server* _Server);
int weather_server_init_ptr(Weather_Server** _ServerPtr);

void weather_server_dispose(Weather_Server* _Server);
void weather_server_dispose_ptr(Weather_Server** _ServerPtr);

#endif
