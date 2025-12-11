#ifndef __WEATHER_API_H__
#define __WEATHER_API_H__


/* ******************************************************************* */
/* ************************* WEATHER API V1 ************************** */
/* ******************************************************************* */

#include "HTTPStatusCodes.h"
#include "http_parser.h"
#include "api/geo_parser.h"
#include "weather_parser.h"

#include <errno.h>
#include <string.h>

typedef enum {
  WEATHER_API_INITIALIZING,
  WEATHER_API_DECIDING_ENDPOINT,
  WEATHER_API_VALIDATING,
  WEATHER_API_IDLING,
  WEATHER_API_RESPONDING,
  WEATHER_API_DISPOSING,
  WEATHER_API_ERROR

}WeatherApiStates;




#define API_ENDPOINT_ROOT "/api/v1"
#define MAX_GEO_RESULTS   8
typedef struct Weather_API Weather_API;

typedef int (*instance_on_api_finish)(Weather_API* _API);
typedef int (*endpoint_function)(Weather_API* _API);
typedef struct
{
  const char*         path;
  HTTPMethod          method;
  endpoint_function  endpoint_func;

} Weather_API_Endpoint; // Array of definitions in weather_api.c

struct Weather_API
{
  HTTP_Request*                   http_request;
  HTTP_Response*                  http_response;

  Geos*                           geos; // This contains the weather structs as well

  int              endpoint;
  instance_on_api_finish          on_api_finish;
};


/* ---------------------- Interface ----------------------- */

int weather_api_init_ptr(Weather_API** _Weather_Req_Ptr, HTTP_Request* _HTTP_Req, HTTP_Response* _HTTP_Res, instance_on_api_finish _on_finish);

/** Takes the path from HTTP_Request and validates it, runs relevant endpoint functions */
HttpStatus_Code weather_api_handle_endpoint(Weather_API* _Request);

void weather_api_dispose_ptr(Weather_API** _Request_Ptr);

/* -------------------------------------------------------- */

#endif
