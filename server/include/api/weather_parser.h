#ifndef __WEATHER_PARSER_H__
#define __WEATHER_PARSER_H__

/* ******************************************************************* */
/* ************************* WEATHER PARSER ************************** */
/* ******************************************************************* */

#include "../http/http_parser.h"
#include "../../../utils/include/file_utils.h"
#include "../../../utils/include/time_utils.h"
#include "../../../libs/include/md5.h"

#include "meteo.h"
#include "bigdatacloud.h"

#include <stdint.h>
#include <stdlib.h>

#define CACHE_DIR "data/cache/"
#define CITY_COORD_BUFFER_LENGTH 12 // How many max chars we take from latitude+longitude param strings. 12 will for example allow -111.111111
#define CITY_NAME_BUFFER_LENGTH 128 // How many max chars we take from name param string

typedef enum
{
  OPEN_METEO

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

typedef struct
{
  Weather*    weather;
  Forecast*   forecast;

  const char* cache_path;
  const char* country;
  const char* city;
  const char* locality;
  const char* timezone; // local timezone, ex: "Europe/Stockholm"

  char        timezone_gmt[7]; // local timezone, ex: "GMT+12"
  
  float       lat;
  float       lon;

  char        country_code[3]; // two-char country code, ex: "SE"

} Location;


/* ---------------------- Interface ----------------------- */

/** Heap init for data structs. To init its weather/forecast struct, pass true as that arg */
int weather_parser_init_ptr(Location** _L_Ptr, bool _weather, bool _forecast);

/**
 * Pre-reqs: Location must be inited */
int weather_parser_get_location_by_coords(Location* _Location, float _lat, float _lon);

int weather_parser_get_weather(Location* _Location, bool _forecast, ExternalWeatherAPI _ExtAPI);

/** Builds a json formatted string from struct members 
 * Saves it to cache file as well */
char* weather_parser_build_json_weather(Weather* _Weather);
// char* weather_parser_build_json_forecast(Forecast* _Forecast);
char* weather_parser_build_json_location(Location* _Location);

/** Takes a string and tries to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful, 0 if none parsed and -1 on error */
int weather_parser_lat_lon(const char* _val, float* _target_coord);

/** Heap dispose for data structs. To skip one, pass NULL as argument */
void weather_parser_dispose_ptr(Location** _C_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr);

/* -------------------------------------------------------- */


#endif
