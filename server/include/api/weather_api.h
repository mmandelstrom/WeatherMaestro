#ifndef __WEATHER_API_H__
#define __WEATHER_API_H__


/* ******************************************************************* */
/* ************************* WEATHER API V1 ************************** */
/* ******************************************************************* */

#include "weather_parser.h"

#include <errno.h>

#define API_ENDPOINT_ROOT "/api/v1"
#define CACHE_DIR "./data/cache/"

typedef enum
{
  ENDPOINT_WEATHER_GET,
  ENDPOINT_FORECAST_GET,

  ENDPOINT_CITIES_LIST,
  ENDPOINT_CITIES_COORDS,
  ENDPOINT_CITIES_ADD,
  ENDPOINT_CITIES_REMOVE,

  ENDPOINT_INVALID, // This must be last (defines count)

} WeatherAPIEndpoint;

typedef struct
{
  const char*         path;
  HTTPMethod          method;
  WeatherAPIEndpoint  endpoint;

} Weather_API_Endpoint; // Array of definitions in weather_api.c

typedef struct
{
  HTTP_Request*       http_request;
  HTTP_Response*      http_response;

  City*               city; // This contains the weather structs as well

  WeatherAPIEndpoint  endpoint;

} Weather_API;


/* ---------------------- Interface ----------------------- */

int weather_api_init_ptr(Weather_API** _Weather_Req_Ptr, HTTP_Request* _HTTP_Req, HTTP_Response* _HTTP_Res);

/** Takes the path from HTTP_Request and validates it, runs relevant endpoint functions */
int weather_api_handle_endpoint(Weather_API* _Request);

void weather_api_dispose_ptr(Weather_API** _Request_Ptr);

/* -------------------------------------------------------- */

#endif
