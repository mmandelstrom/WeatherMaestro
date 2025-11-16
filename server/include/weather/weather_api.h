#ifndef __WEATHER_API_H__
#define __WEATHER_API_H__


/* ******************************************************************* */
/* ************************** WEATHER API **************************** */
/* ******************************************************************* */

#include <time.h>

typedef enum
{
  CITIES_LIST,
  CITY_GET,
  CITY_ADD,
  CITY_REMOVE,

  WEATHER_GET,
  FORECAST_GET

} WeatherServerRequestType;

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


#endif
