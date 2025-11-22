#ifndef __WEATHER_API_H__
#define __WEATHER_API_H__


/* ******************************************************************* */
/* ************************* WEATHER API V1 ************************** */
/* ******************************************************************* */

#include "../../include/http/http_parser.h"
#include <time.h>
#include <errno.h>

#define API_ENDPOINT_ROOT "/api/v1"
#define COORD_BUFFER_LENGTH 12 // How many max chars we take from latitude+longitude params. 12 will for example allow -111.111111

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
  WeatherAPIEndpoint  endpoint;
  HTTPMethod          method;
  const char*         path;

} Weather_API_Endpoint;

typedef struct
{
  time_t      timestamp;
  int         weather_code;

  double      temperature; 
  const char* temperature_unit; 

  double      windspeed; 
  const char* windspeed_unit;

  int         winddirection;
  const char* winddirection_cardinal;
  /* const char* winddirection_unit; */

  double      precipitation;
  const char* precipitation_unit;

} Weather;

typedef struct
{
  int         count;
  Weather*    weather;

} Forecast;

typedef struct
{
  const char* name;
  char        country[2];
  float       lat;
  float       lon;

} City;

typedef struct
{
  WeatherAPIEndpoint  endpoint;

  HTTP_Request*       http_request;
  City*               city;
  Weather*            current_weather;
  Forecast*           forecast_weather;

  char*               response_body;

} Weather_Request;

int weather_api_init_ptr(Weather_Request** _Weather_Req_Ptr, HTTP_Request* _HTTP_Req);

int weather_api_handle_endpoint(Weather_Request* _Request);

void weather_api_dispose_ptr(Weather_Request** _Request_Ptr);

#endif
