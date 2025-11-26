#include "../../include/api/weather_parser.h"


/* ---------------------- Internal functions ----------------------- */

int weather_parser_parse_weather_meteo(Weather* _Weather, Meteo_Weather* _M_Weather);

const char* weather_parser_build_weather_json(Weather* _Weather);

/* ----------------------------------------------------------------- */


int weather_parser_init_ptr(City** _C_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr)
{
  if (_C_Ptr != NULL)
  {
    *_C_Ptr = malloc(sizeof(City));
    if (*_C_Ptr == NULL)
    {
      perror("malloc");
      return -1;
    }

    memset(*_C_Ptr, 0, sizeof(City));
  }
  if (_W_Ptr != NULL)
  {

    *_W_Ptr = malloc(sizeof(Weather));
    if (*_W_Ptr == NULL)
    {
      perror("malloc");
      return -2;
    }

    memset(*_W_Ptr, 0, sizeof(Weather));
  }
  if (_F_Ptr != NULL)
  {
    *_F_Ptr = malloc(sizeof(Forecast));
    if (*_F_Ptr == NULL)
    {
      perror("malloc");
      return -3;
    }

    memset(*_F_Ptr, 0, sizeof(Forecast));
  }

  return 0;
}

int weather_parser_get_weather_meteo(City* _City, bool _forecast)
{
  int result;

  /* Init meteo */
  Meteo_Weather* MW;
  result = meteo_init_ptr(&MW);
  if (result != 0)
  {
    perror("meteo_init");
    return -3;
  }

  /* Get fresh Meteo_Weather struct from API */
  result = meteo_get_weather(MW, _City->lat, _City->lon, _forecast);
  if (result != 0)
  {
    perror("meteo_get_weather");
    meteo_dispose_ptr(&MW);
    return -4;
  }

  printf("---Meteo_Weather---\ntemperature: %f %s\n", MW->temperature_2m, MW->temperature_2m_unit);
  printf("latitude: %f\n", MW->latitude);

  result = weather_parser_parse_weather_meteo(_City->weather, MW);
  if (result != 0)
  {
    perror("weather_parser_parse_meteo_weather");
    meteo_dispose_ptr(&MW);
    return -5;
  }

  /* printf("---Weather---\ntemperature: %f %s\n", _City->weather->temperature, _City->weather->temperature_unit); */

  meteo_dispose_ptr(&MW);

  return 0;
}

int weather_parser_parse_weather_meteo(Weather* _Weather, Meteo_Weather* _M_Weather)
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
    return -1;
  }

  _Weather->timestamp              = parse_iso_string_to_epoch(_M_Weather->timestamp);

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

  return 0;
}

const char* weather_parser_build_weather_json(Weather* _Weather)
{

  cJSON* Json_Root = cJSON_CreateObject();
  cJSON* Json_Weather = cJSON_CreateObject();
  cJSON* Json_Units = cJSON_CreateObject();

  cJSON_AddNumberToObject(Json_Root, "timestamp", _Weather->timestamp);
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

  char* json_str = cJSON_Print(Json_Root);

  cJSON_Delete(Json_Root);

  return json_str;
  
}

/** Takes a string and tries to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful, 0 if none parsed and -1 on error */
int weather_parser_lat_lon(const char* _val, float* _target_coord)
{
  if (_target_coord == NULL)
    return -1;

  char coord[COORD_BUFFER_LENGTH];
  char* endptr; // for strtof

  int val_len = strlen(_val);
  errno = 0; // reset errno
  if (val_len > COORD_BUFFER_LENGTH)
  {
    strncpy(coord, _val, COORD_BUFFER_LENGTH);
    coord[COORD_BUFFER_LENGTH-1] = '\0';
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


void weather_parser_dispose_ptr(City** _C_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr)
{
  /* Dispose of City */
  if (_C_Ptr != NULL)
  {
    if (*_C_Ptr != NULL)
    {
      if ((*_C_Ptr)->forecast != NULL)
      {
        //TODO: dispose each individual Weather struct
        free((*_C_Ptr)->forecast);
        (*_C_Ptr)->forecast = NULL;
      }
      if ((*_C_Ptr)->weather != NULL)
      {
        if ((*_C_Ptr)->weather->temperature_unit   != NULL)  
        {
          free((void*)(*_C_Ptr)->weather->temperature_unit);
          (*_C_Ptr)->weather->temperature_unit = NULL;
        }
        if ((*_C_Ptr)->weather->windspeed_unit     != NULL)    
        {
          free((void*)(*_C_Ptr)->weather->windspeed_unit);
          (*_C_Ptr)->weather->windspeed_unit = NULL;
        }
        if ((*_C_Ptr)->weather->precipitation_unit != NULL)
        {
          free((void*)(*_C_Ptr)->weather->precipitation_unit);
          (*_C_Ptr)->weather->precipitation_unit = NULL;
        }
        if ((*_C_Ptr)->weather->winddirection_unit != NULL)
        {
          free((void*)(*_C_Ptr)->weather->winddirection_unit);
          (*_C_Ptr)->weather->winddirection_unit = NULL;
        }

        free((*_C_Ptr)->weather);
        (*_C_Ptr)->weather = NULL;
      }
      if ((*_C_Ptr)->name != NULL)
      {
        free((void*)(*_C_Ptr)->name);
        (*_C_Ptr)->name = NULL;
      }
      free(*_C_Ptr);
      *_C_Ptr = NULL;
    }
    _C_Ptr = NULL;
  }

  /* Dispose of Forecast */
  if (_F_Ptr != NULL)
  {
    if (*_F_Ptr != NULL)
    {
      //TODO: dispose each individual Weather struct
      free(*_F_Ptr);
      *_F_Ptr = NULL;
    }
    _F_Ptr = NULL;
  }

  /* Dispose of Weather */
  if (_W_Ptr != NULL)
  {
    if (*_W_Ptr != NULL)
    {
      free(*_W_Ptr);
      *_W_Ptr = NULL;
    }
    _W_Ptr = NULL;
  }

}
