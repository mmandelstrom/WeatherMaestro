#ifndef __WEATHER_API_H__
#define __WEATHER_API_H__


/* ******************************************************************* */
/* ************************* WEATHER API V1 ************************** */
/* ******************************************************************* */

#include "http_parser.h"
#include "api/geo_parser.h"
#include "weather_parser.h"

#include <errno.h>
#include <string.h>

#define API_ENDPOINT_ROOT "/api/v1"
#define MAX_GEO_RESULTS   8

typedef enum
{
  ENDPOINT_WEATHER_GET,
  ENDPOINT_FORECAST_GET,

  ENDPOINT_GEO_LIST,
  ENDPOINT_GEO_GET,
  ENDPOINT_GEO_ADD,
  ENDPOINT_GEO_REMOVE,

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

  Geos*               geos; // This contains the weather structs as well

  WeatherAPIEndpoint  endpoint;

} Weather_API;


/* ---------------------- Interface ----------------------- */

int weather_api_init_ptr(Weather_API** _Weather_Req_Ptr, HTTP_Request* _HTTP_Req, HTTP_Response* _HTTP_Res);

/** Takes the path from HTTP_Request and validates it, runs relevant endpoint functions */
int weather_api_handle_endpoint(Weather_API* _Request);

void weather_api_dispose_ptr(Weather_API** _Request_Ptr);

/* -------------------------------------------------------- */

#endif
