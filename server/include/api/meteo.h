/* ******************************************************************* */
/* ****************************** METEO ****************************** */
/* ******************************************************************* */
/* Ahhh, good old open-meteo */

#include "../http/http_client.h"

#include <stdio.h>

#define METEO_CACHE_DIR "../../data/cache/meteo/"
#define METEO_BASE_URL "https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f%s"
#define METEO_CURRENT_WEATHER_QUERY "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"
#define METEO_FORECAST_WEATHER_QUERY "&hourly=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=GMT"


const char* meteo_get_weather(float _lat, float _lon);
