#ifndef __WEATHER_PARSER_H__
#define __WEATHER_PARSER_H__

/* ******************************************************************* */
/* ************************* WEATHER PARSER ************************** */
/* ******************************************************************* */

#include "HTTPStatusCodes.h"
#include "file_utils.h"
#include "md5.h"
#include "scheduler.h"
#include "time_utils.h"

#include "api/meteo.h"

#include <error.h>
#include <stdint.h>
#include <stdlib.h>

#define CACHE_DIR "data/cache/"

/* Pre-hashed cache filename definitions */
#define CACHE_FILENAME_CURRENT "lat%.3f_lon%.3f_current"
#define CACHE_FILENAME_FORECAST "lat%.3f_lon%.3f_forecast"

typedef enum {
  WEATHER_PARSER_INITIALIZING,
  WEATHER_PARSER_CHECKING_CACHE,
  WEATHER_PARSER_CALLING_EXT_API,
  WEATHER_PARSER_IDLING,
  WEATHER_PARSER_PARSING,
  WEATHER_PARSER_RESPONDING,
  WEATHER_PARSER_DISPOSING,
  WEATHER_PARSER_ERROR

} WeatherParserState;

typedef enum {
  OPEN_METEO_WEATHER,
  EXT_WEATHER_API_COUNT,

} ExternalWeatherAPI;

typedef struct {
  const char *cache_path;
  int count;

} Forecast;

typedef struct {
  time_t timestamp;
  time_t update_interval;

  float temperature;
  float precipitation;
  float windspeed;

  const char winddirection_cardinal[4];

  unsigned short winddirection_azimuth;
  uint8_t wmo_code;

} Weather_Values;

typedef struct {
  const char *cache_path;
  const char *temperature_unit;
  const char *windspeed_unit;
  const char *precipitation_unit;
  const char *winddirection_unit;

  Weather_Values *values;

  double elevation;

  float latitude;
  float longitude;

  unsigned int count;

} Weather;

/** Callback when parser has finished, full json will be written to
 * *_json_output_ptr */
typedef void (*parser_on_finish)(void *_context, char **_json_output_ptr);

/** Main function to be called as part of task work */
/* typedef int (*parser_function)(float _lat, float _lon, void* _context,
 * parser_on_finish _on_finish); */

typedef struct {
  /* parser_function     function; */
  parser_on_finish on_finish;
  void *context;
  Scheduler_Task *task;

  Weather *weather;
  Meteo *meteo;
  const char *cache_path;


  float latitude;
  float longitude;

  WeatherParserState state;
  ExternalWeatherAPI external_api;

  bool forecast;

} Weather_Parser;

/* ---------------------- Interface ----------------------- */

/** Taskwork starter, _forecast=false gets current weather */
int weather_parser_init_ptr(Weather_Parser **_WP_Ptr, void *_context,
                            parser_on_finish _on_finish, float _lat, float _lon,
                            bool _forecast);

/** Taskwork ender */
void weather_parser_dispose_ptr(Weather_Parser **_WP_Ptr);

/* -------------------------------------------------------- */

#endif
