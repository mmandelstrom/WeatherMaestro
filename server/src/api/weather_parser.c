#include "../../include/api/weather_parser.h"

/* Pre-hashed cache filename definitions */
#define CACHE_FILENAME_WEATHER  "lat%.3f_lon%.3f_current"
#define CACHE_FILENAME_FORECAST "lat%.3f_lon%.3f_forecast"

/* ---------------------- Internal functions ----------------------- */

/** Writes to heap, free'd by caller */
char* weather_parser_get_cache_filepath(float _lat, float _lon, bool _forecast);

/** Checks how long ago timestamp in cachefile was 
  Returns true if file with same path and within time interval exists */
bool weather_parser_recent_cache_exists(const char* _filepath, int _interval, bool _forecast);

/* WEATHER */

/** Builds a Weather or Forecast struct using external API */
int weather_parser_get_weather_from_api_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI);

/** Parses an open-meteo struct to our Weather struct */
int weather_parser_parse_meteo_weather(Weather* _Weather, Meteo_Weather* _M_Weather);

/** Builds a Weather or Forecast struct from cache file */
int weather_parser_get_weather_from_cache(Weather* _Weather, const char* _filepath);

/** Builds a json formatted string from struct members 
 * Saves it to cache file as well */
char* weather_parser_build_json_weather(Weather* _Weather);

/* FORECAST */

int weather_parser_get_forecast_from_api_by_coords(Forecast* _Forecast, float _lat, float _lon, ExternalWeatherAPI _ExtAPI);

int weather_parser_parse_meteo_forecast(Forecast* _Forecast, Meteo_Forecast* _M_Forecast);

char* weather_parser_build_json_forecast(Forecast* _Forecast);

/* ----------------------------------------------------------------- */


/* ********************** WEATHER & FORECAST *********************** */

/** Heap init for data structs. Pass NULL if you don't want one or the other */
int weather_parser_init_ptr(Weather** _W_Ptr, Forecast** _F_Ptr)
{
  /* Init Weather struct */
  if (_W_Ptr != NULL)
  {
    *_W_Ptr = malloc(sizeof(Weather));
    if (*_W_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_W_Ptr, 0, sizeof(Weather));
  }

  /* Init Forecast struct */
  if (_F_Ptr != NULL)
  {
    *_F_Ptr = malloc(sizeof(Forecast));
    if (*_F_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_F_Ptr, 0, sizeof(Forecast));
  }

  return SUCCESS;
}

char* weather_parser_get_cache_filepath(float _lat, float _lon, bool _forecast)
{
  int filename_len;
  char filename_buf[256];
  if (!_forecast)
  {
    filename_len = snprintf(filename_buf, 256,
        CACHE_FILENAME_WEATHER,
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

void weather_parser_dispose_ptr(Weather** _W_Ptr, Forecast** _F_Ptr)
{

  /* Dispose of Forecast */
  if (_F_Ptr != NULL)
  {
    if (*_F_Ptr != NULL)
    {
      if ((*_F_Ptr)->cache_path != NULL)
      {
        free((void*)(*_F_Ptr)->cache_path);
        (*_F_Ptr)->cache_path = NULL;
      }
      //TODO: dispose each individual Weather struct
      // weather_parser_dispose_ptr(NULL, [forecastindexptr]->weather, NULL)
      free(*_F_Ptr);
      *_F_Ptr = NULL;
    }
    _F_Ptr = NULL;
  }

  /* Dispose of Weather */
  if (_W_Ptr != NULL)
  {
    if ((*_W_Ptr) != NULL)
    {
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
int weather_parser_get_weather_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI, char** _json_output_ptr)
{
  if (_Weather == NULL)
    return -1;

  int interval = 0;

  if (_ExtAPI == OPEN_METEO_WEATHER)
  {
    interval = 900;
  }
  else
    return 501; // not implemented

  _Weather->cache_path = weather_parser_get_cache_filepath(_lat, _lon, false);

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
    if (weather_parser_get_weather_from_api_by_coords(_Weather, _lat, _lon, _ExtAPI) != 0)
      return -3;
    *_json_output_ptr = weather_parser_build_json_weather(_Weather);
  }

  return 0;
}

int weather_parser_get_weather_from_api_by_coords(Weather* _Weather, float _lat, float _lon, ExternalWeatherAPI _ExtAPI)
{
  int result;
  int interval;

  if (_ExtAPI == OPEN_METEO_WEATHER)
  {
    /* Init meteo */
    Meteo_Weather* MW;
    result = meteo_init_ptr(&MW);
    if (result != 0)
    {
      perror("meteo_init");
      return -2;
    }

    /* Get fresh Meteo_Weather struct from API */
    result = meteo_get_weather(MW, _lat, _lon);
    if (result != 0)
    {
      perror("meteo_get_weather");
      meteo_dispose_ptr(&MW);
      return -3;
    }

    printf("---Meteo_Weather---\ntemperature: %f %s\n", MW->temperature_2m, MW->temperature_2m_unit);
    printf("latitude: %f\n", MW->latitude);
    printf("windspeed: %f %s\n", MW->wind_speed_10m, MW->wind_speed_10m_unit);

    result = weather_parser_parse_meteo_weather(_Weather, MW);
    if (result != 0)
    {
      perror("weather_parser_parse_meteo_weather");
      meteo_dispose_ptr(&MW);
      return -4;
    }

    meteo_dispose_ptr(&MW);

    return 0;
  }

  return -1;
}

/* This will probably never be needed */
int weather_parser_get_weather_from_cache(Weather* _Weather, const char* _filepath)
{
  const char* weather_json = read_file_to_string(_filepath);
  if (weather_json == NULL)
    return -1;

  cJSON* Json_Root = cJSON_Parse(weather_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"meteo json error %s\n", error_pointer);
    }
    free((void*)weather_json);
    return -2;
  }
  free((void*)weather_json);

  cJSON* Weather = cJSON_GetObjectItemCaseSensitive(Json_Root, "weather");
  if (Weather == NULL){
    fprintf(stderr, "'weather' section missing in cache json\n");
    cJSON_Delete(Json_Root);
    return -2;
  }
  cJSON* Units = cJSON_GetObjectItemCaseSensitive(Json_Root, "units");
  if (Units == NULL){
    fprintf(stderr, "'current_units' section missing in cache json\n");
    cJSON_Delete(Json_Root);
    return -3;
  }

  const char* timestamp_str = json_get_string(Json_Root, "timestamp");

  _Weather->timestamp              = parse_iso_datetime_string_to_epoch(timestamp_str);

  _Weather->latitude               = json_get_double(Json_Root, "latitude");
  _Weather->longitude              = json_get_double(Json_Root, "longitude");
                                                            
  _Weather->temperature            = json_get_int(Weather, "temperature");
  _Weather->precipitation          = json_get_int(Weather, "precipitation");
  _Weather->windspeed              = json_get_int(Weather, "windspeed");
  _Weather->winddirection_azimuth  = json_get_int(Weather, "winddirection");
  _Weather->wmo_code               = json_get_int(Weather, "wmo_code");

  _Weather->temperature_unit       = strdup(json_get_string(Units, "temperature_unit")); 
  _Weather->windspeed_unit         = strdup(json_get_string(Units, "windspeed_unit")); 
  _Weather->precipitation_unit     = strdup(json_get_string(Units, "precipitation_unit"));
  _Weather->winddirection_unit     = strdup(json_get_string(Units, "winddirection_unit"));

  if (_Weather->temperature_unit   == NULL ||
      _Weather->windspeed_unit     == NULL ||
      _Weather->precipitation_unit == NULL ||
      _Weather->winddirection_unit == NULL)
  {
    fprintf(stderr, "One or more strings couldn't be parsed from meteo json\n");
    cJSON_Delete(Json_Root);
    return -3;
  }

  cJSON_Delete(Json_Root);

  return 0;
}

int weather_parser_parse_meteo_weather(Weather* _Weather, Meteo_Weather* _M_Weather)
{

  /* Assign string members */
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
  
  /* _Weather->winddirection_cardinal = strdup(weather_parser_get_cardinal_direction(_M_Weather->wind_direction_10m)); */
  
  _Weather->winddirection_azimuth  = _M_Weather->wind_direction_10m;
  _Weather->wmo_code               = _M_Weather->weather_code;

  printf("_M_Weather->latitude: %f", _M_Weather->latitude);
  printf("_Weather->latitude: %f", _Weather->latitude);

  return SUCCESS;
}

char* weather_parser_build_json_weather(Weather* _Weather)
{
  if (!_Weather) {
    return NULL;
  }

  cJSON* Json_Root = cJSON_CreateObject();
  cJSON* Json_Weather = cJSON_CreateObject();
  cJSON* Json_Units = cJSON_CreateObject();

  const char* iso_timestamp = parse_epoch_to_iso_datetime_string(&_Weather->timestamp);

  cJSON_AddStringToObject(Json_Root, "timestamp", iso_timestamp);
  cJSON_AddNumberToObject(Json_Root, "latitude", _Weather->latitude);
  cJSON_AddNumberToObject(Json_Root, "longitude", _Weather->longitude);

  cJSON_AddNumberToObject(Json_Weather, "temperature", _Weather->temperature);
  cJSON_AddNumberToObject(Json_Weather, "precipitation", _Weather->precipitation);
  cJSON_AddNumberToObject(Json_Weather, "windspeed", _Weather->windspeed);
  cJSON_AddNumberToObject(Json_Weather, "winddirection", _Weather->winddirection_azimuth);
  cJSON_AddNumberToObject(Json_Weather, "wmo_code", _Weather->wmo_code);

  cJSON_AddStringToObject(Json_Units, "temperature_unit", _Weather->temperature_unit);
  cJSON_AddStringToObject(Json_Units, "windspeed_unit", _Weather->windspeed_unit);
  cJSON_AddStringToObject(Json_Units, "precipitation_unit", _Weather->precipitation_unit);
  cJSON_AddStringToObject(Json_Units, "winddirection_unit", _Weather->winddirection_unit);

  cJSON_AddItemToObject(Json_Root, "units", Json_Units);
  cJSON_AddItemToObject(Json_Root, "weather", Json_Weather);

  char* json_str = cJSON_Print(Json_Root); // Uses realloc and ends up in heap

  if (write_string_to_file(json_str, _Weather->cache_path) != 0)
    fprintf(stderr, "Failed to write string \"%p\" to cache \"%s\"\n", json_str, _Weather->cache_path); 

  free((void*)iso_timestamp);
  cJSON_Delete(Json_Root);

  return json_str;
}

/************************ FORECAST *************************/

int weather_parser_get_forecast_by_coords(Forecast* _Forecast, float _lat, float _lon, ExternalWeatherAPI _ExtAPI, char** _json_output_ptr)
{
  if (_Forecast == NULL)
    return ERR_INVALID_ARG;

  int interval = 0;

  if (_ExtAPI == OPEN_METEO_WEATHER)
  {
    interval = 86400;
  }
  else
    return ERR_INVALID_ARG; // not implemented

  _Forecast->cache_path = weather_parser_get_cache_filepath(_lat, _lon, true);

  if (weather_parser_recent_cache_exists(_Forecast->cache_path, 
        interval, 
        false))
  {
    printf("Getting weather from cache\n");
    *_json_output_ptr = read_file_to_string(_Forecast->cache_path);
  }
  else
  {
    printf("Getting weather from API\n");
    if (weather_parser_get_forecast_from_api_by_coords(_Forecast, _lat, _lon, _ExtAPI) != 0)
      return -3;
    *_json_output_ptr = weather_parser_build_json_weather(_Forecast);
  }


  return SUCCESS;
}

int weather_parser_get_forecast_from_api_by_coords(Forecast* _Forecast, float _lat, float _lon, ExternalWeatherAPI _ExtAPI)
{

  return SUCCESS;
}
int weather_parser_parse_meteo_forecast(Forecast* _Forecast, Meteo_Forecast* _M_Forecast)
{

  return SUCCESS;
}

char* weather_parser_build_json_forecast(Forecast* _Forecast)
{

  return NULL;
}
