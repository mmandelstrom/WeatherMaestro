#ifndef __WEATHER_PARSER_H__
#define __WEATHER_PARSER_H__

/* ******************************************************************* */
/* ************************* WEATHER PARSER ************************** */
/* ******************************************************************* */

#include "../http/http_parser.h"
#include "../../../utils/include/file_utils.h"
#include "../../../utils/include/time_utils.h"

#include "meteo.h"

#include <stdint.h>
#include <stdlib.h>

#define COORD_BUFFER_LENGTH 12 // How many max chars we take from latitude+longitude param strings. 12 will for example allow -111.111111

typedef struct
{
  const char*     temperature_unit; 
  const char*     windspeed_unit;
  const char*     precipitation_unit;
  const char*     winddirection_unit;

  time_t          timestamp;

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
  Weather*    weather;
  int         count;

} Forecast;

typedef struct
{
  Weather*    weather;
  Forecast*   forecast;

  const char* name;

  char        timezone_gmt[7]; // local timezone, ex: "GMT+12"
  
  float       lat;
  float       lon;

  char        country[3]; // two-char country code, ex: "SE"

} City;


/* ---------------------- Interface ----------------------- */

/** Heap init for data structs. To skip one, pass NULL as argument */
int weather_parser_init_ptr(City** _C_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr);

/** Builds a Weather or Forecast struct using open-meteo API
 * Pre-reqs: _City->lat & _City->lon must be set and _City->weather must be inited
 * If _forecast is true then _City->forecast must be inited instead */
int weather_parser_get_weather_meteo(City* _City, bool _forecast);

/** Builds a json formatted string from built struct */
const char* weather_parser_build_weather_json(Weather* _Weather);
const char* weather_parser_build_forecast_json(Forecast* _Forecast);

/** Takes a string and tries to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful, 0 if none parsed and -1 on error */
int weather_parser_lat_lon(const char* _val, float* _target_coord);

/** Heap dispose for data structs. To skip one, pass NULL as argument */
void weather_parser_dispose_ptr(City** _C_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr);

/* -------------------------------------------------------- */


#endif
