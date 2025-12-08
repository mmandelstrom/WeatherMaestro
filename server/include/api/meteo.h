#ifndef __METEO_H__
#define __METEO_H__

/* ******************************************************************* */
/* ****************************** METEO ****************************** */
/* ******************************************************************* */
/* Ahhh, good old open-meteo */

#include "../http/http_client.h"
#include "../../../libs/include/cJSON.h"
#include "../../../utils/include/json_utils.h"
#include "../../../utils/include/curl.h"

#include <stdio.h>
#include <time.h>

#define METEO_BASE_URL "https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f%s"
#define METEO_CURRENT_WEATHER_QUERY "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"
#define METEO_FORECAST_WEATHER_QUERY "&hourly=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"

typedef struct
{
  char        timestamp[17];

  double      temperature_2m; 
  double      wind_speed_10m; 
  double      precipitation;
  double      elevation;
  double      generationtime_ms;

  const char* timezone_abbreviation;

  const char* temperature_2m_unit; 
  const char* wind_speed_10m_unit;
  const char* precipitation_unit;
  const char* elevation_unit;
  const char* wind_direction_10m_unit;

  float       latitude;
  float       longitude;

  int         utc_offset_seconds;
  int         interval;
  int         is_day;
  int         weather_code;
  int         wind_direction_10m;

} Meteo_Weather;

typedef struct
{
  const char* temperature_2m_unit; 
  const char* wind_speed_10m_unit;
  const char* precipitation_unit;
  const char* elevation_unit;
  const char* wind_direction_10m_unit;

  float       latitude;
  float       longitude;

  Meteo_Weather** weathers;
  unsigned short  count;

} Meteo_Forecast;

typedef struct
{

} Meteo_Geo;

/* TODO: overlook forecast saving for interface function, maybe remove Forecast struct and instead use pointers in all Weather struct values, then save count and just loop through them all as arrays... Or simply add a new function for saving forecast */

/* ---------------------- Interface ----------------------- */

/* Only init the one you need, pass NULL to the others*/
int meteo_init_ptr(Meteo_Geo** _MG_Ptr, Meteo_Weather** _MW_Ptr, Meteo_Forecast** _MF_Ptr);

int meteo_get_weather(Meteo_Weather* _MW, float _lat, float _lon);
int meteo_get_forecast(Meteo_Forecast* _MF, float _lat, float _lon);

int meteo_get_geo(Meteo_Geo* _MF, float _lat, float _lon, int _count);

int meteo_init_ptr(Meteo_Geo** MG_Ptr, Meteo_Weather** _MW_Ptr, Meteo_Forecast** _MF_Ptr);

/* -------------------------------------------------------- */

#endif
