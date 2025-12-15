#include "api/weather_parser.h"
#include "api/meteo.h"
#include "scheduler.h"
#include <error.h>

/* ---------------------- INTERNAL DEFS ----------------------- */

/** Writes to heap, free'd by caller */
char* weather_parser_get_cache_filepath(float _lat, float _lon, bool _forecast);
/** Checks how long ago timestamp in cachefile was 
  Returns true if file with same path and within time interval exists */
bool weather_parser_recent_cache_exists(const char* _filepath, int _interval, bool _forecast);

/* CURRENT WEATHER */

/** Builds a Weather or Weather struct using external API */
int weather_parser_get_weather_from_api_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI);
/** Parses an open-meteo struct to our Weather struct */
int weather_parser_parse_meteo_current(Weather* _Weather, Meteo_Current* _M_Weather);
/** Builds a json formatted string from struct members 
 * Saves it to cache file as well */
char* weather_parser_build_json_weather(Weather* _Weather);

/* FORECAST WEATHER */

int weather_parser_get_forecast_from_api_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI);
int weather_parser_parse_meteo_hourly(Weather* _Weather, Meteo_Hourly* _M_Weather);
char* weather_parser_build_json_forecast(Weather* _Weather);

/* TASKWORK */

void weather_parser_on_ext_api_finish(void* _context, void* _ext_api);

void weather_parser_taskwork(void* _context, uint64_t _montime);
WeatherParserState weather_parser_worktask_check_cache(Weather_Parser* _Parser);
WeatherParserState weather_parser_worktask_call_ext_api(Weather_Parser* _Parser);
WeatherParserState weather_parser_worktask_parse_api_response(Weather_Parser* _Parser);

/* ----------------------------------------------------------------- */

int weather_parser_init_ptr(Weather_Parser** _Parser_Ptr, void* _context, parser_on_finish _on_finish, float _lat, float _lon, bool _forecast)
{
  if (!_Parser_Ptr)
  {
    return ERR_INVALID_ARG;
  }

  *_Parser_Ptr = calloc(1, sizeof(Weather_Parser));
  if (*_Parser_Ptr == NULL)
  {
    perror("calloc");
    return ERR_NO_MEMORY;
  }

  (*_Parser_Ptr)->latitude = _lat;
  (*_Parser_Ptr)->longitude = _lon;
  (*_Parser_Ptr)->context = _context;
  (*_Parser_Ptr)->on_finish = _on_finish;
  (*_Parser_Ptr)->forecast = _forecast;
  (*_Parser_Ptr)->state = WEATHER_PARSER_INITIALIZING;
  (*_Parser_Ptr)->task = scheduler_create_task(*_Parser_Ptr, weather_parser_taskwork);
  if (!(*_Parser_Ptr)->task) 
  {
    weather_parser_dispose_ptr(_Parser_Ptr);
    return ERR_INTERNAL;
  }

  return SUCCESS;
}

void weather_parser_on_ext_api_finish(void* _context, void* _ext_api)
{
  Weather_Parser* Parser = (Weather_Parser*)_context;
  Parser->meteo = (Meteo*)_ext_api;

  Parser->state = WEATHER_PARSER_PARSING;
}

void weather_parser_taskwork(void* _context, uint64_t _montime)
{

  if (!_context) 
    return;
  
  Weather_Parser* Parser = (Weather_Parser*)_context;

  int result;
  switch (Parser->state)
  {
    case WEATHER_PARSER_INITIALIZING:
    {
      printf("WEATHER_PARSER_INITIALIZING\n");

    } break;

    case WEATHER_PARSER_CHECKING_CACHE:
    {
      printf("WEATHER_PARSER_CHECKING_CACHE\n");
      Parser->state = weather_parser_worktask_check_cache(Parser);

    } break;

    case WEATHER_PARSER_CALLING_EXT_API:
    {
      printf("WEATHER_PARSER_CALLING_EXT_API\n");
      Parser->state = weather_parser_worktask_call_ext_api(Parser); 

    } break;

    case WEATHER_PARSER_IDLING:
      break;

    case WEATHER_PARSER_PARSING:
    {
      printf("WEATHER_PARSER_PARSING\n");
      Parser->state = weather_parser_worktask_parse_api_response(Parser);

    } break;

    case WEATHER_PARSER_DISPOSING:
    {
      printf("WEATHER_PARSER_DISPOSING\n");
      weather_parser_dispose_ptr(&Parser);

    } break;

    case WEATHER_PARSER_ERROR:
    {
      printf("WEATHER_PARSER_ERROR\n");
      fprintf(stderr, "weather_parser_taskwork failed, Weather_Parser: %p\n", Parser);
      weather_parser_dispose_ptr(&Parser);

    } break;

    default:
      break;
  }
}

WeatherParserState weather_parser_worktask_check_cache(Weather_Parser* _Parser)
{
  ExternalWeatherAPI ExtAPI;
  int interval = 0;
  int i;
  for (i = 0; i < EXT_WEATHER_API_COUNT; i++)
  {
    ExtAPI = i;

    if (ExtAPI == OPEN_METEO_WEATHER)
    {
      interval = 900;
    }
    else
    {
      return WEATHER_PARSER_ERROR;
    }

    const char* cache_path = weather_parser_get_cache_filepath(_Parser->latitude, _Parser->longitude, false);

    if (cache_path == NULL)
    {
      perror("weather_parser_get_cache_filepath");
      return WEATHER_PARSER_ERROR;
    }
    else if (weather_parser_recent_cache_exists(cache_path, interval, false))
    {
      printf("Getting weather from cache\n");

      char* json_output = read_file_to_string(cache_path);
      free((void*)cache_path);
      if (json_output == NULL)
      {
        perror("read_file_to_string");
        return WEATHER_PARSER_ERROR;
      }
      else
      {
        _Parser->on_finish(_Parser->context, &json_output);
      }
    }
    else
    {
      printf("Getting weather from API\n");

      return WEATHER_PARSER_CALLING_EXT_API;
    }
  }
  return WEATHER_PARSER_ERROR;
}
WeatherParserState weather_parser_worktask_call_ext_api(Weather_Parser* _Parser)
{
  int result;
  int interval;

  int count = 0;
  if (_Parser->forecast)
    count = 7 * 24;

  Weather* W;
  if (weather_parser_init_weather_ptr(&W, count))
    return WEATHER_PARSER_ERROR;

  if (_Parser->external_api == OPEN_METEO_WEATHER)
  {
    /* Init meteo */
    Meteo* M;
    Meteo_Current* MC;
    result = meteo_init_ptr(&M, NULL, &MC, NULL, weather_parser_on_ext_api_finish, _Parser);
    if (result != 0)
    {
      perror("meteo_init");
      return -2;
    }

    /* Get fresh Meteo_Current struct from API */
    /* result = meteo_get_weather(MC, _Parser->latitude, _Parser->longitude);
    if (result != 0)
    {
      perror("meteo_get_weather");
      meteo_dispose_ptr(NULL, &MC, NULL);
      return WEATHER_PARSER_ERROR;
    } */

    return WEATHER_PARSER_IDLING;
  }
  else
  {
    return WEATHER_PARSER_ERROR;
  }

}
WeatherParserState weather_parser_worktask_parse_api_response(Weather_Parser* _Parser)
{

  int result = ERR_INTERNAL;
  if (_Parser->external_api == OPEN_METEO_WEATHER)
  {
    if (_Parser->meteo->current != NULL)
      result = weather_parser_parse_meteo_current(_Parser->weather, _Parser->meteo->current);

    if (_Parser->meteo->hourly != NULL)
      result = weather_parser_parse_meteo_hourly(_Parser->weather, _Parser->meteo->hourly);
  }
  else
  {
    return WEATHER_PARSER_ERROR;
  }

  meteo_dispose_ptr(NULL, &_Parser->meteo->current, &_Parser->meteo->hourly);

  return WEATHER_PARSER_RESPONDING; 
}

WeatherParserState weather_parser_worktask_parse_respond(Weather_Parser* _Parser)
{
  char* response;

  
   response = weather_parser_build_json_weather(_Parser->weather);
  _Parser->on_finish(_Parser->context, &response);
  
  return WEATHER_PARSER_DISPOSING;
}
/* ********************** CURRENT & FORECAST *********************** */

/** Heap init for Weather and Weather_Values structs. 
 * _count decides how many value structs to init (for example 1 for current weather only) */
int weather_parser_init_weather_ptr(Weather** _W_Ptr, unsigned int _count)
{
  if (!_W_Ptr)
  {
    return ERR_INVALID_ARG;
  }

  *_W_Ptr = calloc((size_t)_count, sizeof(Weather));
  if (*_W_Ptr == NULL)
  {
    perror("malloc");
    return ERR_NO_MEMORY;
  }

  (*_W_Ptr)->values = calloc((size_t)_count, sizeof(Weather_Values));
  if ((*_W_Ptr)->values == NULL)
  {
    perror("malloc");
    return ERR_NO_MEMORY;
  }

  (*_W_Ptr)->count = _count;

  return SUCCESS;
}

char* weather_parser_get_cache_filepath(float _lat, float _lon, bool _forecast)
{
  int filename_len;
  char filename_buf[256];
  if (!_forecast)
  {
    filename_len = snprintf(filename_buf, 256,
        CACHE_FILENAME_CURRENT,
        _lat,
        _lon);
  }
  else
  {
    filename_len = snprintf(filename_buf, 256,
        CACHE_FILENAME_FORECAST,
        _lat,
        _lon);
  }
  const char* hashed_filename = MD5_HashToString(filename_buf, filename_len);

  char* file_ext = ".json";
  int filepath_len = strlen(CACHE_DIR) + strlen(hashed_filename) + strlen(file_ext);
  char* full_filepath = malloc(filepath_len + 1);
  snprintf(full_filepath, filepath_len, "%s%s%s",
      CACHE_DIR, hashed_filename, file_ext);

  return full_filepath;
}

bool weather_parser_recent_cache_exists(const char* _filepath, int _interval, bool _forecast)
{
  if (_filepath == NULL)
    return false;

  const char* cache_contents = read_file_to_string(_filepath);
  if (cache_contents == NULL)
    return false;

  cJSON* Json_Root = cJSON_Parse(cache_contents);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"cJSON_Parse error %s\n", error_pointer);
    }
    free((void*)cache_contents);
    return false;
  }
  free((void*)cache_contents);

  time_t timestamp_epoch;
  /* Different json placements for timestamp in weather/forecast */
  if (_forecast) // TODO: Implement when forecast is a thing
  {
    cJSON_Delete(Json_Root);
    return false;
  }
  else
  {
    const char* timestamp_str = json_get_string(Json_Root, "timestamp");
    timestamp_epoch = parse_iso_datetime_string_to_epoch(timestamp_str);
    if (timestamp_epoch == (time_t)-1)
    {
      cJSON_Delete(Json_Root);
      return false;
    }
  }
  /* Compare timestamp from json with time now and see if interval has bee reached */
  time_t now = time(NULL);
  double timediff = difftime(now, timestamp_epoch);

  if (timediff < (double)_interval)
  {
    cJSON_Delete(Json_Root);
    return true;
  }

  cJSON_Delete(Json_Root);
  return false;
}

void weather_parser_dispose_weather_ptr(Weather** _W_Ptr)
{
  if (_W_Ptr != NULL)
  {
    if ((*_W_Ptr) != NULL)
    {
      if ((*_W_Ptr)->values != NULL)
      {
        free((*_W_Ptr)->values);
        (*_W_Ptr)->values = NULL;
      }

      if ((*_W_Ptr)->cache_path != NULL)
      {
        free((void*)(*_W_Ptr)->cache_path);
        (*_W_Ptr)->cache_path = NULL;
      }
      if ((*_W_Ptr)->temperature_unit   != NULL)  
      {
        free((void*)(*_W_Ptr)->temperature_unit);
        (*_W_Ptr)->temperature_unit = NULL;
      }
      if ((*_W_Ptr)->windspeed_unit     != NULL)    
      {
        free((void*)(*_W_Ptr)->windspeed_unit);
        (*_W_Ptr)->windspeed_unit = NULL;
      }
      if ((*_W_Ptr)->precipitation_unit != NULL)
      {
        free((void*)(*_W_Ptr)->precipitation_unit);
        (*_W_Ptr)->precipitation_unit = NULL;
      }
      if ((*_W_Ptr)->winddirection_unit != NULL)
      {
        free((void*)(*_W_Ptr)->winddirection_unit);
        (*_W_Ptr)->winddirection_unit = NULL;
      }

      free((*_W_Ptr));
      (*_W_Ptr) = NULL;
    }
    _W_Ptr = NULL;
  }
}

/************************ CURRENT WEATHER *************************/

/** Builds a Weather struct using cache or external API */

int weather_parser_parse_meteo_weather(Weather_Parser* _Parser, Meteo_Current* _M_Weather)
{

  /* Assign string members */
  _Parser->weather->temperature_unit       = strdup(_M_Weather->temperature_2m_unit);
  _Parser->weather->windspeed_unit         = strdup(_M_Weather->wind_speed_10m_unit);
  _Parser->weather->precipitation_unit     = strdup(_M_Weather->precipitation_unit);
  _Parser->weather->winddirection_unit     = strdup(_M_Weather->wind_direction_10m_unit);

  if ( _Parser->weather->temperature_unit   == NULL || 
       _Parser->weather->windspeed_unit     == NULL || 
       _Parser->weather->precipitation_unit == NULL || 
       _Parser->weather->winddirection_unit == NULL)
  {
    perror("Failed to duplicate meteo strings to weather struct");
    return ERR_INTERNAL;
  }

  _Parser->weather->values[0].timestamp              = parse_iso_datetime_string_to_epoch(_M_Weather->timestamp);

  _Parser->latitude                                  = _M_Weather->latitude;
  _Parser->longitude                                 = _M_Weather->longitude;
  _Parser->weather->values[0].temperature            = _M_Weather->temperature_2m;
  _Parser->weather->values[0].precipitation          = _M_Weather->precipitation;
  _Parser->weather->values[0].windspeed              = _M_Weather->wind_speed_10m; 
  
  /* _Weather->winddirection_cardinal = strdup(weather_parser_get_cardinal_direction(_M_Weather->wind_direction_10m)); */
  
  _Parser->weather->values[0].winddirection_azimuth  = _M_Weather->wind_direction_10m;
  _Parser->weather->values[0].wmo_code               = _M_Weather->weather_code;

  printf("_M_Weather->latitude: %f", _M_Weather->latitude);
  printf("_Weather->latitude: %f", _Parser->latitude);

  return SUCCESS;
}

/* int weather_parser_parse_meteo_forecast(Weather* _Weather, Meteo_Current* _M_Weather)
{

  if (_M_Weather->count > _Weather->count)
  {
    _Weather->weather = realloc(_Weather->weather, sizeof(Weather*) * _M_Weather->count);

  }
  
  for (int i = 0; i < _Weather->count; i++)

  _Weather->temperature_unit       = strdup(_M_Weather->temperature_2m_unit);
  _Weather->windspeed_unit         = strdup(_M_Weather->wind_speed_10m_unit);
  _Weather->precipitation_unit     = strdup(_M_Weather->precipitation_unit);
  _Weather->winddirection_unit     = strdup(_M_Weather->wind_direction_10m_unit);

  if (_Weather->temperature_unit   == NULL || 
      _Weather->windspeed_unit     == NULL || 
      _Weather->precipitation_unit == NULL || 
      _Weather->winddirection_unit == NULL)
  {
    perror("Failed to duplicate meteo strings to weather struct");
    return ERR_INTERNAL;
  }

  _Weather->timestamp              = parse_iso_datetime_string_to_epoch(_M_Weather->timestamp);

  _Weather->latitude               = _M_Weather->latitude;
  _Weather->longitude              = _M_Weather->longitude;
  _Weather->temperature            = _M_Weather->temperature_2m;
  _Weather->precipitation          = _M_Weather->precipitation;
  _Weather->windspeed              = _M_Weather->wind_speed_10m; 
  
  
  _Weather->winddirection_azimuth  = _M_Weather->wind_direction_10m;
  _Weather->wmo_code               = _M_Weather->weather_code;

  printf("_M_Weather->latitude: %f", _M_Weather->latitude);
  printf("_Weather->latitude: %f", _Weather->latitude);

  return SUCCESS;
} */

char* weather_parser_build_json_current(Weather* _Weather)
{
  if (!_Weather || !_Weather->values)
    return NULL;

  Weather_Values* Values = _Weather->values;

  cJSON* Json_Root = cJSON_CreateObject();
  cJSON* Json_Weather = cJSON_CreateObject();
  cJSON* Json_Units = cJSON_CreateObject();

  if (!Json_Root || !Json_Weather || !Json_Units)
    return NULL;

  const char* iso_timestamp = parse_epoch_to_iso_datetime_string(&Values[0].timestamp);

  cJSON_AddStringToObject(Json_Root, "timestamp", iso_timestamp);
  cJSON_AddNumberToObject(Json_Root, "latitude", _Weather->latitude);
  cJSON_AddNumberToObject(Json_Root, "longitude", _Weather->longitude);

  cJSON_AddNumberToObject(Json_Weather, "temperature", Values[0].temperature);
  cJSON_AddNumberToObject(Json_Weather, "precipitation", Values[0].precipitation);
  cJSON_AddNumberToObject(Json_Weather, "windspeed", Values[0].windspeed);
  cJSON_AddNumberToObject(Json_Weather, "winddirection", Values[0].winddirection_azimuth);
  cJSON_AddNumberToObject(Json_Weather, "wmo_code", Values[0].wmo_code);

  cJSON_AddStringToObject(Json_Units, "temperature_unit", _Weather->temperature_unit);
  cJSON_AddStringToObject(Json_Units, "windspeed_unit", _Weather->windspeed_unit);
  cJSON_AddStringToObject(Json_Units, "precipitation_unit", _Weather->precipitation_unit);
  cJSON_AddStringToObject(Json_Units, "winddirection_unit", _Weather->winddirection_unit);

  cJSON_AddItemToObject(Json_Root, "units", Json_Units);
  cJSON_AddItemToObject(Json_Root, "weather", Json_Weather);

  char* json_str = cJSON_Print(Json_Root); // Uses realloc and ends up in heap

  if (write_string_to_file(json_str, _Weather->cache_path) != 0)
    fprintf(stderr, "Failed to write string \"%p\" to cache \"%p\"\n", json_str, _Weather->cache_path); 

  free((void*)iso_timestamp);
  cJSON_Delete(Json_Root);

  return json_str;
}

/************************ FORECAST WEATHER *************************/

int weather_parser_get_forecast_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI, char** _json_output_ptr)
{
  if (_Weather == NULL)
    return ERR_INVALID_ARG;

  int interval = 0;

  if (_ExtAPI == OPEN_METEO_WEATHER)
  {
    interval = 86400;
  }
  else
    return ERR_INVALID_ARG; // not implemented

  _Weather->cache_path = weather_parser_get_cache_filepath(_lat, _lon, true);

  if (weather_parser_recent_cache_exists(_Weather->cache_path, 
        interval, 
        false))
  {
    printf("Getting weather from cache\n");
    *_json_output_ptr = read_file_to_string(_Weather->cache_path);
  }
  else
  {
    printf("Getting weather from API\n");
    if (weather_parser_get_forecast_from_api_by_coords(_Weather, _lat, _lon, _ExtAPI) != 0)
      return -3;
    *_json_output_ptr = weather_parser_build_json_forecast(_Weather);
  }


  return SUCCESS;
}

int weather_parser_get_forecast_from_api_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI)
{

  return SUCCESS;
}
int weather_parser_parse_meteo_forecast(Weather* _Weather, Meteo_Current* _M_Weather)
{

  return SUCCESS;
}

char* weather_parser_build_json_forecast(Weather* _Weather)
{
  if (!_Weather || !_Weather->values)
    return NULL;

  Weather_Values* Values = _Weather->values;

  cJSON* Json_Root = cJSON_CreateObject();
  cJSON* Json_Weather = cJSON_CreateObject();
  cJSON* Json_Units = cJSON_CreateObject();

  if (!Json_Root || !Json_Weather || !Json_Units)
    return NULL;


  cJSON_AddNumberToObject(Json_Root,    "latitude",           _Weather->latitude);
  cJSON_AddNumberToObject(Json_Root,    "longitude",          _Weather->longitude);

  int i;
  for (i = 0; i < _Weather->count; i++)
  {
    const char* iso_timestamp = parse_epoch_to_iso_datetime_string(&Values[i].timestamp);

    cJSON_AddStringToObject(Json_Root,    "timestamp",          iso_timestamp);

    cJSON_AddNumberToObject(Json_Weather, "temperature",        Values[i].temperature);
    cJSON_AddNumberToObject(Json_Weather, "precipitation",      Values[i].precipitation);
    cJSON_AddNumberToObject(Json_Weather, "windspeed",          Values[i].windspeed);
    cJSON_AddNumberToObject(Json_Weather, "winddirection",      Values[i].winddirection_azimuth);
    cJSON_AddNumberToObject(Json_Weather, "wmo_code",           Values[i].wmo_code);

    free((void*)iso_timestamp);
  }

  cJSON_AddStringToObject(Json_Units,   "temperature_unit",   _Weather->temperature_unit);
  cJSON_AddStringToObject(Json_Units,   "windspeed_unit",     _Weather->windspeed_unit);
  cJSON_AddStringToObject(Json_Units,   "precipitation_unit", _Weather->precipitation_unit);
  cJSON_AddStringToObject(Json_Units,   "winddirection_unit", _Weather->winddirection_unit);

  cJSON_AddItemToObject(Json_Root, "units", Json_Units);
  cJSON_AddItemToObject(Json_Root, "weather", Json_Weather);

  char* json_str = cJSON_Print(Json_Root); // Uses realloc and ends up in heap

  if (write_string_to_file(json_str, _Weather->cache_path) != 0)
    fprintf(stderr, "Failed to write string \"%p\" to cache \"%p\"\n", json_str, _Weather->cache_path); 

  cJSON_Delete(Json_Root);

  return json_str;
}
