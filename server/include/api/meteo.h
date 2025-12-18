#ifndef __METEO_H__
#define __METEO_H__

/* ******************************************************************* */
/* ****************************** METEO ****************************** */
/* ******************************************************************* */
/* Ahhh, good old open-meteo */

#include "cJSON.h"
#include "http_client.h"
#include "json_utils.h"
#include "curl.h"

#include <stdio.h>
#include <time.h>

#define METEO_BASE_URL "http://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f%s"
#define METEO_CURRENT_WEATHER_QUERY "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"
#define METEO_FORECAST_WEATHER_QUERY "&hourly=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"


typedef struct
{
  char        timestamp[17];

  double      temperature_2m; 
  double      wind_speed_10m; 
  double      precipitation;

  int         is_day;
  int         weather_code;
  int         wind_direction_10m;

} Meteo_Weather_Values;

typedef struct
{
  const char*           timezone_abbreviation;

  const char*           temperature_2m_unit; 
  const char*           wind_speed_10m_unit;
  const char*           precipitation_unit;
  const char*           wind_direction_10m_unit;
  const char*           elevation_unit;

  double                generationtime_ms;
  double                elevation;

  float                 latitude;
  float                 longitude;

  int                   utc_offset_seconds;
  int                   interval;

  Meteo_Weather_Values* values;

} Meteo_Current;


typedef struct
{
  char*                 timestamp[17];

  const char*           timezone_abbreviation;
  const char*           temperature_2m_unit; 
  const char*           wind_speed_10m_unit;
  const char*           precipitation_unit;
  const char*           wind_direction_10m_unit;
  const char*           elevation_unit;

  double                generationtime_ms;
  double                elevation;

  float                 latitude;
  float                 longitude;

  int                   utc_offset_seconds;
  int                   interval;
  int                   is_day;

  Meteo_Weather_Values* values;
  unsigned int          count;

} Meteo_Hourly;

typedef struct
{

} Meteo_Geo;

typedef struct Meteo Meteo;

typedef void (*on_ext_api_finish)(void* _context, void* _ext_api);

typedef struct Meteo
{
  void*             context;
  on_ext_api_finish on_finish;

  Meteo_Geo*        geo;
  Meteo_Current*    current;
  Meteo_Hourly*     hourly;

  char*             http_response;

} Meteo;

/* TODO: overlook forecast saving for interface function, maybe remove Forecast struct and instead use pointers in all Weather struct values, then save count and just loop through them all as arrays... Or simply add a new function for saving forecast */

/* ---------------------- Interface ----------------------- */

int meteo_get_geo(Meteo** _M_Ptr, float _lat, float _lon, int _count, on_ext_api_finish _on_finish, void* _context);
int meteo_get_weather_current(Meteo** _M_Ptr, float _lat, float _lon, on_ext_api_finish _on_finish, void* _context);
int meteo_get_weather_hourly(Meteo** _M_Ptr, float _lat, float _lon, on_ext_api_finish _on_finish, void* _context);

void meteo_dispose_ptr(Meteo** _M_Ptr, Meteo_Geo** _MG_Ptr, Meteo_Current** _MW_Ptr, Meteo_Hourly** _MF_Ptr);

/* -------------------------------------------------------- */

#endif
