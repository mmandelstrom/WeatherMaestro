#ifndef __WEATHER_API_H__
#define __WEATHER_API_H__


/* ******************************************************************* */
/* ************************* WEATHER API V1 ************************** */
/* ******************************************************************* */

#include "HTTPStatusCodes.h"
#include "http_parser.h"
#include "api/geo_parser.h"
#include "scheduler.h"
#include "weather_parser.h"

#include <errno.h>
#include <string.h>


#define API_ENDPOINT_ROOT "/api/v1"
#define MAX_GEO_RESULTS   8

typedef enum 
{
  WEATHER_API_INITIALIZING,
  WEATHER_API_DECIDING_ENDPOINT,
  WEATHER_API_VALIDATING,
  WEATHER_API_IDLING,
  WEATHER_API_RESPONDING,
  WEATHER_API_DISPOSING,
  WEATHER_API_ERROR

} WeatherApiState;


typedef struct Weather_API Weather_API;

typedef int (*endpoint_function)(Weather_API* _API);

typedef struct
{
  const char*         path;
  HTTPMethod          method;
  endpoint_function   endpoint_func;

} Weather_API_Endpoint;

/* Callback for when API has finished processing request and response */
typedef int (*on_api_finish)(void* _context);

struct Weather_API
{
  HTTP_Request*           http_request;
  HTTP_Response*          http_response;

  Geos*                   geos;
  Forecast*               forecast;

  Scheduler_Task*         task;
  on_api_finish           on_api_finish; 
  void*                   context; // context for on_api_finish 

  endpoint_function       endpoint_func;

  WeatherApiState         state;
};


/* ---------------------- Interface ----------------------- */

int weather_api_init_ptr(Weather_API** _Weather_Req_Ptr, HTTP_Request* _HTTP_Req, HTTP_Response* _HTTP_Res, on_api_finish _on_finish, void* _context);

// that's all folks, _on_finish will be called when taskwork is done
// don't forget to dispose!

void weather_api_dispose_ptr(Weather_API** _Request_Ptr);

/* -------------------------------------------------------- */

#endif
