#ifndef __WEATHER_PARSER_H__
#define __WEATHER_PARSER_H__

/* ******************************************************************* */
/* ************************* WEATHER PARSER ************************** */
/* ******************************************************************* */

#include "../../../utils/include/file_utils.h"
#include "../../../utils/include/time_utils.h"
#include "../../../libs/include/md5.h"

#include "meteo.h"
#include "bigdatacloud.h"
#include "nominatim.h"

#include <stdint.h>
#include <stdlib.h>

#define CACHE_DIR "data/cache/"

typedef enum
{
  OPEN_METEO_WEATHER,

} ExternalWeatherAPI;

typedef struct
{
  const char*     cache_path;
  const char*     temperature_unit; 
  const char*     windspeed_unit;
  const char*     precipitation_unit;
  const char*     winddirection_unit;

  time_t          timestamp;
  time_t          update_interval;
  
  /* lat+lon differ in precision from city lat+lon, depending on where data was gathered from */
  float           latitude; 
  float           longitude;
  float           temperature; 
  float           precipitation;
  float           windspeed; 

  const char      winddirection_cardinal[4];

  unsigned short  winddirection_azimuth;
  uint8_t         wmo_code;

} Weather;

typedef struct
{
  const char* cache_path;
  Weather*    weather;
  int         count;

} Forecast;

/* ---------------------- Interface ----------------------- */

/** Heap init for data structs. Pass NULL if you don't want one or the other */
int weather_parser_init_ptr(Weather** _W_Ptr, Forecast** _F_Ptr);

/** Builds a Weather or Forecast struct using cache or external API */
int weather_parser_get_weather_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI, char** _json_output_ptr);
int weather_parser_get_forecast_by_coords(Forecast* _Foracast, float _lat, float _lon, ExternalWeatherAPI _ExtAPI, char** _json_output_ptr);

/** Heap dispose for data structs. To skip one, pass NULL as argument */
void weather_parser_dispose_ptr(Weather** _W_Ptr, Forecast** _F_Ptr);

/* -------------------------------------------------------- */


#endif
