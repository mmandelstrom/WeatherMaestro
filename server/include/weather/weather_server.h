#ifndef __WEATHER_SERVER_H__
#define __WEATHER_SERVER_H__


/* ******************************************************************* */
/* ************************* WEATHER SERVER ************************** */
/* ******************************************************************* */

#include "weather_instance.h"
#include "../http/http_server.h"
#include "../scheduler.h"
#include "../../../utils/include/linked_list.h"

#define DATA_DIR "data/"

typedef enum
{
  WEATHER_SERVER_INIT,
  WEATHER_SERVER_IDLE,
  WEATHER_SERVER_CONNECTING,
  WEATHER_SERVER_CONNECTED,
  WEATHER_SERVER_ERROR,
  WEATHER_SERVER_DISPOSING

} WeatherServerState;

typedef enum
{
  WEATHER_SERVER_ERROR_NONE = 0,
  WEATHER_SERVER_ERROR_INIT_FAILED,
  
} WeatherServerErrorState;

typedef struct
{
	HTTP_Server             http_server;
	HTTP_Server_Connection* http_connection; // temporary, hands over to instance
  Scheduler_Task*         task;
  Linked_List*            instances;
  WeatherServerState      state;

} Weather_Server;


int weather_server_init(Weather_Server* _Server);
int weather_server_init_ptr(Weather_Server** _Server_Ptr);
void weather_server_dispose(Weather_Server* _Server);
void weather_server_dispose_ptr(Weather_Server** _Server_Ptr);

#endif
