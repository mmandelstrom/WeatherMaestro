/* ******************************************************************* */
/* ****************************** METEO ****************************** */
/* ******************************************************************* */
/* Ahhh, good old open-meteo */

#include "../http/http_client.h"
#include "../../../libs/include/cJSON.h"
#include "../../../utils/include/json_utils.h"

#include <stdio.h>
#include <time.h>

#define METEO_CACHE_DIR "./data/cache/"
#define METEO_BASE_URL "https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f%s"
#define METEO_CURRENT_WEATHER_QUERY "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"
#define METEO_FORECAST_WEATHER_QUERY "&hourly=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"

typedef struct
{
  char*       timestamp;
  int         utc_offset_seconds;
  char*       timezone_abbreviation;
  double      generationtime_ms;
  int         interval;

  float       latitude;
  float       longitude;

  int         is_day;
  int         weather_code;

  double      elevation;
  const char* elevation_unit;

  double      temperature; 
  const char* temperature_unit; 

  double      windspeed; 
  const char* windspeed_unit;

  int         winddirection;
  const char* winddirection_unit;

  double      precipitation;
  const char* precipitation_unit;

} Meteo_Weather;


const char* meteo_get_weather_json(float _lat, float _lon);

int meteo_parse_json(const char* _json_path, Meteo_Weather* _Meteo_Weather);
