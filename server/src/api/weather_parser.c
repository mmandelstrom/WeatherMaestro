#include "../../include/api/weather_parser.h"

#define CACHE_FILENAME_LOCATION "_lat%.3f_lon%.3f_location"
#define CACHE_FILENAME_WEATHER "_lat%.3f_lon%.3f_current"
#define CACHE_FILENAME_FORECAST "_lat%.3f_lon%.3f_forecast"

/* ---------------------- Internal functions ----------------------- */

int weather_parser_parse_meteo_weather(Weather* _Weather, Meteo_Weather* _M_Weather);
int weather_parser_parse_bigdatacloud_geo(Location* _Location, Bigdatacloud_Geo* _BDC_Geo);

/** Builds a Weather or Forecast struct using open-meteo API
 * Pre-reqs: _Location->lat & _Location->lon must be set and _Location->weather must be inited
 * If _forecast is true then _Location->forecast must be inited instead */
int weather_parser_get_weather_from_api(Location* _Location, ExternalWeatherAPI _ExtAPI, bool _forecast);

char* weather_parser_get_weather_cache_filepath(Location* _Location, bool _forecast);

/** Builds a Weather or Forecast struct from cache file
 * Pre-reqs: Location must have name, lat and lon
 * Returns 0 if succesful, -1 if no recent cache file found (it can still exist) */
int weather_parser_get_weather_from_cache(Location* _Location, const char* _filepath, bool _forecast);

char* weather_parser_build_weather_json(Weather* _Weather);

/**/
bool weather_parser_location_cache_exists(const char* _filepath, const char* _name, float _lat, float _lon);
/** Checks how long ago timestamp in cachefile was 
  Returns true if file with same path and within time interval exists */
bool weather_parser_recent_weather_cache_exists(const char* _filepath, int _interval, bool _forecast);

/** Returns the full filepath for a Location's Weather/Forecast cache 
 * Pre-reqs: Location must have name, lat and lon set 
 * Writes to heap, free'd by caller */
char* weather_parser_get_cache_filepath_location(Location* _Location);
char* weather_parser_get_cache_filepath_weather(Location* _Location, bool _forecast);

/* ----------------------------------------------------------------- */


int weather_parser_init_ptr(Location** _L_Ptr, bool _weather, bool _forecast)
{
  if (_L_Ptr != NULL)
  {
    *_L_Ptr = malloc(sizeof(Location));
    if (*_L_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_L_Ptr, 0, sizeof(Location));

    /* Init Location->weather struct */
    if (_weather)
    {
      (*_L_Ptr)->weather = malloc(sizeof(Weather));
      if ((*_L_Ptr)->weather == NULL)
      {
        perror("malloc");
        return ERR_NO_MEMORY;
      }
      memset((*_L_Ptr)->weather, 0, sizeof(Weather));
    }

    /* Init Location->forecast struct */
    if (_forecast)
    {
      (*_L_Ptr)->forecast = malloc(sizeof(Forecast));
      if ((*_L_Ptr)->forecast == NULL)
      {
        perror("malloc");
        return ERR_NO_MEMORY;
      }
      memset((*_L_Ptr)->forecast, 0, sizeof(Forecast));
    }
  }

  return SUCCESS;
}


/************************ WEATHER PARSING *************************/

/** Looks in cache first, if not recent enough call API for update 
 * Pre-reqs: Location and Location->weather/Location->forecast must be inited */
int weather_parser_get_weather(Location* _Location, bool _forecast, ExternalWeatherAPI _ExtAPI)
{
  printf("_Location: %p, _Location->weather: %p\n", _Location, _Location->weather); 
  if (_Location == NULL || _Location->weather == NULL)
    return -1;

  int interval = 0;

  if (_ExtAPI == OPEN_METEO)
  {
    if (_forecast)
      interval = 86400;
    else
      interval = 900;
  }

  if (_Location->city == NULL)
    return -1;

  _Location->weather->cache_path = weather_parser_get_cache_filepath_weather(_Location, _forecast);

  if (weather_parser_recent_weather_cache_exists(_Location->weather->cache_path, 
        interval, 
        _forecast))
  {
    printf("Getting weather from cache\n");
    if (weather_parser_get_weather_from_cache(_Location, _Location->weather->cache_path, _forecast) != 0)
      return -2;
  }
  else
  {
    printf("Getting weather from API\n");
    if (weather_parser_get_weather_from_api(_Location, _ExtAPI, _forecast) != 0)
      return -3;
  }

  return 0;
}

int weather_parser_get_weather_from_api(Location* _Location, ExternalWeatherAPI _ExtAPI, bool _forecast)
{
  int result;
  int interval;

  if (_ExtAPI == OPEN_METEO)
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
    result = meteo_get_weather(MW, _Location->lat, _Location->lon, _forecast);
    if (result != 0)
    {
      perror("meteo_get_weather");
      meteo_dispose_ptr(&MW);
      return -3;
    }

    printf("---Meteo_Weather---\ntemperature: %f %s\n", MW->temperature_2m, MW->temperature_2m_unit);
    printf("latitude: %f\n", MW->latitude);
    printf("windspeed: %f %s\n", MW->wind_speed_10m, MW->wind_speed_10m_unit);

    result = weather_parser_parse_meteo_weather(_Location->weather, MW);
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

int weather_parser_get_weather_from_cache(Location* _Location, const char* _filepath, bool _forecast)
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

  if (!_forecast)
  {
  const char* timestamp_str = json_get_string(Json_Root, "timestamp");

  _Location->weather->timestamp             = parse_iso_datetime_string_to_epoch(timestamp_str);

  _Location->weather->latitude              = json_get_double(Json_Root, "latitude");
  _Location->weather->longitude             = json_get_double(Json_Root, "longitude");
                                                           
  _Location->weather->temperature           = json_get_int(Json_Root, "temperature");
  _Location->weather->precipitation         = json_get_int(Json_Root, "precipitation");
  _Location->weather->windspeed             = json_get_int(Json_Root, "windspeed");
  _Location->weather->winddirection_azimuth = json_get_int(Json_Root, "winddirection");
  _Location->weather->wmo_code              = json_get_int(Json_Root, "wmo_code");

  _Location->weather->temperature_unit      = strdup(json_get_string(Json_Root, "temperature_unit")); 
  _Location->weather->windspeed_unit        = strdup(json_get_string(Json_Root, "windspeed_unit")); 
  _Location->weather->precipitation_unit    = strdup(json_get_string(Json_Root, "precipitation_unit"));
  _Location->weather->winddirection_unit    = strdup(json_get_string(Json_Root, "winddirection_unit"));
  }

  if (_Location->weather->temperature_unit == NULL ||
      _Location->weather->windspeed_unit == NULL ||
      _Location->weather->precipitation_unit == NULL ||
      _Location->weather->winddirection_unit == NULL)
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
  _Weather->wmo_code               = _M_Weather->weathercode;

  printf("_M_Weather->latitude: %f", _M_Weather->latitude);
  printf("_Weather->latitude: %f", _Weather->latitude);

  return SUCCESS;
}

char* weather_parser_get_cache_filepath_weather(Location* _Location, bool _forecast)
{
  int filename_len;
  char filename_buf[256];
  if (!_forecast)
  {
    filename_len = snprintf(filename_buf, 256,
        CACHE_FILENAME_WEATHER,
        _Location->lat,
        _Location->lon);
  }
  else
  {
    filename_len = snprintf(filename_buf, 256,
        CACHE_FILENAME_FORECAST,
        _Location->lat,
        _Location->lon);
  }
  const char* hashed_filename = MD5_HashToString(filename_buf, filename_len);

  char* file_ext = ".json";
  int filepath_len = strlen(CACHE_DIR) + strlen(hashed_filename) + strlen(file_ext);
  char* full_filepath = malloc(filepath_len + 1);
  snprintf(full_filepath, filepath_len, "%s%s%s",
      CACHE_DIR, hashed_filename, file_ext);

  return full_filepath;
}

bool weather_parser_recent_weather_cache_exists(const char* _filepath, int _interval, bool _forecast)
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

  if (_forecast) // TODO: Implement when forecast is a thing
  {
    cJSON_Delete(Json_Root);
    return false;
  }
  else
  {
    const char* timestamp_str = json_get_string(Json_Root, "timestamp");
    time_t timestamp_epoch = parse_iso_datetime_string_to_epoch(timestamp_str);

    if (timestamp_epoch == (time_t)-1)
    {
      cJSON_Delete(Json_Root);
      return false;
    }

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
}

char* weather_parser_build_json_weather(Weather* _Weather, const char* _cache_path)
{
  if (!_Weather) {
    return "";
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

  if (write_string_to_file(json_str, _cache_path) != 0)
    fprintf(stderr, "Failed to write string \"%p\" to cache \"%s\"\n", json_str, _cache_path); 

  free((void*)iso_timestamp);
  cJSON_Delete(Json_Root);

  return json_str;
}

/************************ LOCATION PARSING *************************/

/* First looks in cache by lat and lon, if none found search via API */
int weather_parser_get_location_by_coords(Location* _Location, float _lat, float _lon)
{
  int result;

  /* Init bigdatacloud */
  Bigdatacloud_Geo* BDC_Geo;
  result = bigdatacloud_init_ptr(&BDC_Geo);
  if (result != 0)
  {
    perror("bigdatacloud_init");
    return -1;
  }

  /* Get fresh Bigdatacloud_Geo struct from API */
  result = bigdatacloud_get_geo_by_coords(BDC_Geo, _Location->lat, _Location->lon);
  if (result != 0)
  {
    perror("bigdatacloud_get_weather");
    bigdatacloud_dispose_ptr(&BDC_Geo);
    return -2;
  }

  printf("---Bigdatacloud Geo---\ncityname: %s | country %s\n lat: %f, lon: %f\n\n", BDC_Geo->city, BDC_Geo->country_name, BDC_Geo->latitude, BDC_Geo->longitude);
  printf("latitude: %f\n", BDC_Geo->latitude);

  result = weather_parser_parse_bigdatacloud_geo(_Location, BDC_Geo);
  if (result != 0)
  {
    perror("weather_parser_parse_bigdatacloud_weather");
    bigdatacloud_dispose_ptr(&BDC_Geo);
    return -3;
  }

  bigdatacloud_dispose_ptr(&BDC_Geo);

  return 0;
}

int weather_parser_parse_bigdatacloud_geo(Location* _Location, Bigdatacloud_Geo* _BDC_Geo)
{
  _Location->lat = _BDC_Geo->latitude; // should these be updated here?
  _Location->lon = _BDC_Geo->longitude;

  /* Assign string members */
  memcpy(_Location->country_code, _BDC_Geo->country_code, 2);
  _Location->country_code[2] = '\0';

  _Location->city         = strdup(_BDC_Geo->city);
  _Location->country      = strdup(_BDC_Geo->country_name);
  _Location->locality     = strdup(_BDC_Geo->locality);

  if (_Location->locality == NULL ||
      _Location->country  == NULL || 
      _Location->city     == NULL)
  {
    perror("Failed to duplicate bigdatacloud strings");
    return -1;
  }

  printf("_BDC_Geo->latitude: %f", _BDC_Geo->latitude);
  printf("_Location->latitude: %f", _Location->lat);

  return 0;
}

char* weather_parser_get_cache_filepath_location(Location* _Location)
{
  int filename_len;
  char filename_buf[256];
  filename_len = snprintf(filename_buf, 256,
        CACHE_FILENAME_LOCATION,
        _Location->lat,
        _Location->lon);
  const char* hashed_filename = MD5_HashToString(filename_buf, filename_len);

  char* file_ext = ".json";
  int filepath_len = strlen(CACHE_DIR) + strlen(hashed_filename) + strlen(file_ext);
  char* full_filepath = malloc(filepath_len + 1);
  snprintf(full_filepath, filepath_len, "%s%s%s",
      CACHE_DIR, hashed_filename, file_ext);

  return full_filepath;
}

/* Uses member "city" to match with _name */
bool weather_parser_location_cache_exists(const char* _filepath, const char* _name, float _lat, float _lon)
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

  const char* json_name = json_get_string(Json_Root, "city");
  float       json_lat  = json_get_double(Json_Root, "latitude");
  float       json_lon  = json_get_double(Json_Root, "longitude");

  if (json_lat == _lat && json_lon == _lon)
  {
    cJSON_Delete(Json_Root);
    return true;
  }

  cJSON_Delete(Json_Root);
  return false;
}


char* weather_parser_build_json_location(Location* _Location)
{
  cJSON* Json_Root = cJSON_CreateObject();

  json_set_double(Json_Root, "latitude", _Location->lat);
  json_set_double(Json_Root, "longitude", _Location->lon);

  json_set_string(Json_Root, "country", _Location->country);
  json_set_string(Json_Root, "country_code", _Location->country_code);
  json_set_string(Json_Root, "city", _Location->city);
  json_set_string(Json_Root, "locality", _Location->city);
  json_set_string(Json_Root, "timezone", _Location->timezone);
  json_set_string(Json_Root, "timezone_gmt", _Location->timezone_gmt);

  char* json_str = cJSON_Print(Json_Root); // Uses realloc and ends up in heap

  cJSON_Delete(Json_Root);

  return json_str;
}

/** Takes a string and tries to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful parse, 0 if none parsed and -1 on error */
int weather_parser_lat_lon(const char* _val, float* _target_coord)
{
  if (_target_coord == NULL)
    return -1;

  char coord[CITY_COORD_BUFFER_LENGTH];
  char* endptr; // for strtof

  int val_len = strlen(_val);
  errno = 0; // reset errno
  if (val_len > CITY_COORD_BUFFER_LENGTH)
  {
    strncpy(coord, _val, CITY_COORD_BUFFER_LENGTH);
    coord[CITY_COORD_BUFFER_LENGTH-1] = '\0';
  } 
  else 
  {
    strncpy(coord, _val, val_len);
    coord[val_len] = '\0';
  }

  float coord_val = strtof(coord, &endptr);
  if (errno != 0 || endptr == coord || *endptr != '\0')
    return -1;

  *_target_coord = coord_val;

  return 1;
}


void weather_parser_dispose_ptr(Location** _L_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr)
{
  /* Dispose of Location */
  if (_L_Ptr != NULL)
  {
    if (*_L_Ptr != NULL)
    {
      if ((*_L_Ptr)->forecast != NULL)
        weather_parser_dispose_ptr(NULL, NULL, &(*_L_Ptr)->forecast);
      if ((*_L_Ptr)->weather != NULL)
        weather_parser_dispose_ptr(NULL, &(*_L_Ptr)->weather, NULL);

      if ((*_L_Ptr)->cache_path != NULL)
      {
        free((void*)(*_L_Ptr)->cache_path);
        (*_L_Ptr)->cache_path = NULL;
      }
      if ((*_L_Ptr)->locality != NULL)
      {
        free((void*)(*_L_Ptr)->locality);
        (*_L_Ptr)->locality = NULL;
      }
      if ((*_L_Ptr)->city != NULL)
      {
        free((void*)(*_L_Ptr)->city);
        (*_L_Ptr)->city = NULL;
      }
      if ((*_L_Ptr)->country != NULL)
      {
        free((void*)(*_L_Ptr)->country);
        (*_L_Ptr)->country = NULL;
      }
      if ((*_L_Ptr)->timezone != NULL)
      {
        free((void*)(*_L_Ptr)->timezone);
        (*_L_Ptr)->timezone = NULL;
      }
      free(*_L_Ptr);
      *_L_Ptr = NULL;
    }
    _L_Ptr = NULL;
  }

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
