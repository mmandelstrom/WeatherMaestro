#include "api/meteo.h"
#include "http_parser.h"
#include <error.h>


/* ---------------------- Internal functions ----------------------- */

/* WEATHER/FORECAST */
const char* meteo_get_weather_json(float _lat, float _lon, bool _forecast);
int meteo_parse_json_weather_current(Meteo_Current* _MW, const char* _json);
int meteo_parse_json_weather_hourly(Meteo_Hourly* _MF, const char* _json);

/* GEO */
// const char* meteo_get_geo_json(float _lat, float _lon);
// int meteo_parse_json_geo(Meteo_Geo* _MG, const char* _json);

/* ----------------------------------------------------------------- */

int meteo_init_ptr(Meteo** _M_Ptr, Meteo_Geo** _MG_Ptr, Meteo_Current** _MW_Ptr, Meteo_Hourly** _MF_Ptr, on_ext_api_finish _on_finish, void* _context)
{
  /* Module struct */
  if (_M_Ptr != NULL)
  {
    *_M_Ptr = calloc(1, sizeof(Meteo_Geo));
    if (*_M_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    (*_M_Ptr)->on_finish = _on_finish;
    (*_M_Ptr)->context = _context;
  }

  /* Geo */
  if (_MG_Ptr != NULL)
  {
    *_MG_Ptr = calloc(1, sizeof(Meteo_Geo));
    if (*_MG_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
  }

  /* Forecast */
  if (_MF_Ptr != NULL)
  {
    *_MF_Ptr = calloc(1, sizeof(Meteo_Hourly));
    if (*_MF_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
  }

  /* Weather */
  if (_MW_Ptr != NULL)
  {
    *_MW_Ptr = calloc(1, sizeof(Meteo_Current));
    if (*_MW_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
  }

  return 0;
}

void on_http_client_finish(void* _context, const char** _response)
{
  Meteo* M = (Meteo*)_context;
  /* if (M->geo != NULL)
    meteo_parse_json_geo(M, const char *_json) */

  if (M->current != NULL)
    meteo_parse_json_weather_current(M->current, *_response);

  if (M->hourly != NULL)
    meteo_parse_json_weather_hourly(M->hourly, *_response);

  M->on_finish(M->context, M);

}

int meteo_get_weather_current(Meteo* _M, float _lat, float _lon)
{
  if (_M->current == NULL)
    return ERR_INTERNAL;

  /* GET meteo weather json */
  const char* meteo_json = meteo_get_weather_json(_lat, _lon, false);
  if (meteo_json == NULL)
  {
    perror("meteo_get_weather_json");
    return -1;
  }

  /* int json_len = strlen(meteo_json);
  for (int i = 0; i < json_len; i++)
    printf("meteo_json[%i]: %i\n", i, (int)meteo_json[i]);
  printf("meteo_json: %s\n\n", meteo_json); */

  /* Parse meteo json to Meteo_Current struct */
  int result = meteo_parse_json_weather_current(_M->current, meteo_json);
  if (result != 0)
  {
    perror("meteo_parse_json");
    free((void*)meteo_json);
    return result;
  }
  free((void*)meteo_json);

  return 0;
}

int meteo_get_weather_hourly(Meteo* _M, float _lat, float _lon)
{
  if (_M->hourly == NULL)
    return ERR_INTERNAL;

  const char* meteo_json = meteo_get_weather_json(_lat, _lon, true);
  if (meteo_json == NULL)
  {
    perror("meteo_get_weather_json");
    return -1;
  }

  int result = meteo_parse_json_weather_hourly(_M->hourly, meteo_json);
  if (result != 0)
  {
    perror("meteo_parse_json");
    free((void*)meteo_json);
    return result;
  }
  free((void*)meteo_json);

  return SUCCESS;
}

int meteo_get_weather_json(Meteo* _M, float _lat, float _lon, bool _forecast)
{
  char url[512];

  if (_forecast)
  {
    int url_len = snprintf(url, 512, 
             METEO_BASE_URL,
             _lat, 
             _lon,
             METEO_FORECAST_WEATHER_QUERY);
    url[url_len] = '\0';
  }
  else
  {
    int url_len = snprintf(url, 512, 
             METEO_BASE_URL,
             _lat, 
             _lon,
             METEO_CURRENT_WEATHER_QUERY);
    url[url_len] = '\0';
  }
  
  HTTP_Client* c = calloc(1, sizeof(HTTP_Client));
  if (!c) {
    return -1;
  }
  HTTPMethod method = HTTP_GET;

  if (http_client_initiate(c, (const char*)url, method, _M->on_finish) != SUCCESS) 
  {
    return -2;
  }

  return 0;
}
const char* meteo_get_weather_json(float _lat, float _lon, bool _forecast)
{
  char url[512];

  if (_forecast)
  {
    int url_len = snprintf(url, 512, 
             METEO_BASE_URL,
             _lat, 
             _lon,
             METEO_FORECAST_WEATHER_QUERY);
    url[url_len] = '\0';
  }
  else
  {
    int url_len = snprintf(url, 512, 
             METEO_BASE_URL,
             _lat, 
             _lon,
             METEO_CURRENT_WEATHER_QUERY);
    url[url_len] = '\0';
  }
  
  HTTP_Client* c = calloc(1, sizeof(HTTP_Client));
  if (!c) {
    return NULL;
  }
  HTTPMethod method = 1;

  if (http_client_initiate(c, (const char*)url, method) != SUCCESS) {
    return NULL;
  }



  /* char* response = malloc(c->tcp_client->data.size + 1);
  if (response == NULL)
    {
      perror("malloc");
      return NULL;
    }
  
   memcpy(response, c->tcp_client->data.addr, c->tcp_client->data.size);
   response[c->tcp_client->data.size] = '\0';
  
   printf("===== Meteo Response JSON =====\n\n%s\n\n", response); */

  return response;
}

int meteo_parse_json_weather_current(Meteo_Current* _MW, const char* _json)
{
  cJSON* Json_Root = cJSON_Parse(_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"meteo json error %s\n", error_pointer);
    }
    return -1;
  }

  cJSON* Cur_Weather = cJSON_GetObjectItemCaseSensitive(Json_Root, "current");
  if (Cur_Weather == NULL){
    fprintf(stderr, "'current' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return -2;
  }
  cJSON* Cur_Weather_Units = cJSON_GetObjectItemCaseSensitive(Json_Root, "current_units");
  if (Cur_Weather_Units == NULL){
    fprintf(stderr, "'current_units' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return -3;
  }

  /* Parse data from json */
  memcpy(_MW->timestamp, json_get_string(Cur_Weather, "time"), 16);
  _MW->timestamp[16] = '\0';

  _MW->latitude                = json_get_double(Json_Root, "latitude");
  _MW->longitude               = json_get_double(Json_Root, "longitude");
  _MW->generationtime_ms       = json_get_double(Json_Root, "generationtime_ms");
  _MW->elevation               = json_get_double(Json_Root, "elevation");

  _MW->precipitation           = json_get_double(Cur_Weather, "precipitation");
  _MW->temperature_2m          = json_get_double(Cur_Weather, "temperature_2m");
  _MW->wind_speed_10m          = json_get_double(Cur_Weather, "wind_speed_10m");

  _MW->wind_direction_10m      = json_get_int(Cur_Weather, "wind_direction_10m");
  _MW->utc_offset_seconds      = json_get_int(Cur_Weather, "utc_offset_seconds");
  _MW->interval                = json_get_int(Cur_Weather, "interval");
  _MW->is_day                  = json_get_int(Cur_Weather, "is_day");
  _MW->weather_code            = json_get_int(Cur_Weather, "weather_code");

  /* Heap allocations */
  _MW->timezone_abbreviation   = strdup(json_get_string(Json_Root, "timezone_abbreviation"));
  _MW->temperature_2m_unit     = strdup(json_get_string(Cur_Weather_Units, "temperature_2m"));
  _MW->elevation_unit          = strdup(json_get_string(Cur_Weather_Units, "elevation"));
  _MW->precipitation_unit      = strdup(json_get_string(Cur_Weather_Units, "precipitation"));
  _MW->wind_speed_10m_unit     = strdup(json_get_string(Cur_Weather_Units, "wind_speed_10m"));
  _MW->wind_direction_10m_unit = strdup(json_get_string(Cur_Weather_Units, "wind_direction_10m"));
  if (_MW->timezone_abbreviation   == NULL ||
      _MW->temperature_2m_unit     == NULL ||
      _MW->elevation_unit          == NULL ||
      _MW->precipitation_unit      == NULL ||
      _MW->wind_speed_10m_unit     == NULL ||
      _MW->wind_direction_10m_unit == NULL)
  {
    fprintf(stderr, "One or more strings couldn't be parsed from meteo json\n");
    cJSON_Delete(Json_Root);
    return -4;
  }

  cJSON_Delete(Json_Root);

  printf("meteo parsed temperature: %f %s\n", _MW->temperature_2m, _MW->temperature_2m_unit);

  return 0;
}

int meteo_parse_json_weather_hourly(Meteo_Hourly* _MF, const char* _json)
{
  cJSON* Json_Root = cJSON_Parse(_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"meteo json error %s\n", error_pointer);
    }
    return -1;
  }

  cJSON* Hourly_Weather = cJSON_GetObjectItemCaseSensitive(Json_Root, "hourly");
  if (Hourly_Weather == NULL){
    fprintf(stderr, "'hourly' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return -2;
  }
  cJSON* Hourly_Weather_Units = cJSON_GetObjectItemCaseSensitive(Json_Root, "hourly_units");
  if (Hourly_Weather_Units == NULL){
    fprintf(stderr, "'hourly_units' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return -3;
  }
  
  /* Parse the the first array to get count*/
  cJSON* Time = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "time");
  int count = cJSON_GetArraySize(Time);
  _MF->count = count;

  if (count != _MF->count)
  {
    printf("Meteo forecast count is different than allocated!\n");
    cJSON_Delete(Json_Root);
    return -4;
  }
  
  /* _MF = realloc(_MF->, count * sizeof(Meteo_Current));
  if (!_MF->weathers)
  {
    perror("realloc");
    cJSON_Delete(Json_Root);
    return -4;
  } */

  cJSON* Temperature   = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "temperature_2m");
  cJSON* Windspeed     = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "wind_speed_10m");
  cJSON* Winddirection = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "wind_direction_10m");
  cJSON* Precipitation = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "precipitation");
  cJSON* Weathercode   = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "weather_code");
  cJSON* Isday         = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "is_day");

  if (Temperature   == NULL ||
      Windspeed     == NULL ||
      Winddirection == NULL ||
      Precipitation == NULL ||
      Weathercode   == NULL ||
      Isday         == NULL)
  {
    fprintf(stderr, "Hourly weather fields missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return -5;
  }

  _MF->temperature_2m     = realloc(_MF->temperature_2m, sizeof(double) * _MF->count);
  _MF->wind_speed_10m     = realloc(_MF->wind_speed_10m, sizeof(double) * _MF->count);
  _MF->precipitation      = realloc(_MF->precipitation, sizeof(double) * _MF->count);
  _MF->elevation          = realloc(_MF->elevation, sizeof(double) * _MF->count);
  _MF->generationtime_ms  = realloc(_MF->generationtime_ms, sizeof(double) * _MF->count);

  _MF->latitude           = realloc(_MF->latitude, sizeof(float) * _MF->count);
  _MF->longitude          = realloc(_MF->longitude, sizeof(float) * _MF->count);

  _MF->utc_offset_seconds = realloc(_MF->utc_offset_seconds, sizeof(int) * _MF->count);
  _MF->interval           = realloc(_MF->interval, sizeof(int) * _MF->count);
  _MF->is_day             = realloc(_MF->is_day, sizeof(int) * _MF->count);
  _MF->weather_code       = realloc(_MF->weather_code, sizeof(int) * _MF->count);
  _MF->wind_direction_10m = realloc(_MF->wind_direction_10m, sizeof(int) * _MF->count);

  

  for (int i = 0; i < count; i++)
  {

    /* Parse data from json */
    memcpy(_MF[i].timestamp, json_get_string(Hourly_Weather, "time"), 16);
    _MF->timestamp[i][16] = '\0';
                                
    _MF->latitude              [i]  = json_get_double(Json_Root, "latitude");
    _MF->longitude             [i]  = json_get_double(Json_Root, "longitude");
    _MF->generationtime_ms     [i]  = json_get_double(Json_Root, "generationtime_ms");
    _MF->elevation             [i]  = json_get_double(Json_Root, "elevation");
                                
    _MF->precipitation         [i]  = json_get_double(Hourly_Weather, "precipitation");
    _MF->temperature_2m        [i]  = json_get_double(Hourly_Weather, "temperature_2m");
    _MF->wind_speed_10m        [i]  = json_get_double(Hourly_Weather, "wind_speed_10m");
                                
    _MF->wind_direction_10m    [i]  = json_get_int(Hourly_Weather, "wind_direction_10m");
    _MF->utc_offset_seconds    [i]  = json_get_int(Hourly_Weather, "utc_offset_seconds");
    _MF->interval              [i]  = json_get_int(Hourly_Weather, "interval");
    _MF->is_day                [i]  = json_get_int(Hourly_Weather, "is_day");
    _MF->weather_code          [i]  = json_get_int(Hourly_Weather, "weather_code");

    /* Heap allocations */
    _MF[i].timezone_abbreviation   = strdup(json_get_string(Json_Root, "timezone_abbreviation"));
    _MF[i].temperature_2m_unit     = strdup(json_get_string(Hourly_Weather_Units, "temperature_2m"));
    _MF[i].elevation_unit          = strdup(json_get_string(Hourly_Weather_Units, "elevation"));
    _MF[i].precipitation_unit      = strdup(json_get_string(Hourly_Weather_Units, "precipitation"));
    _MF[i].wind_speed_10m_unit     = strdup(json_get_string(Hourly_Weather_Units, "wind_speed_10m"));
    _MF[i].wind_direction_10m_unit = strdup(json_get_string(Hourly_Weather_Units, "wind_direction_10m"));
    if (_MF[i].timezone_abbreviation   == NULL ||
        _MF[i].temperature_2m_unit     == NULL ||
        _MF[i].elevation_unit          == NULL ||
        _MF[i].precipitation_unit      == NULL ||
        _MF[i].wind_speed_10m_unit     == NULL ||
        _MF[i].wind_direction_10m_unit == NULL)
    {
      fprintf(stderr, "One or more strings couldn't be parsed from meteo json\n");
      cJSON_Delete(Json_Root);
      return -6;
    }
  }
  

  cJSON_Delete(Json_Root);


  return 0;
}

void meteo_dispose_ptr(Meteo_Geo** _MG_Ptr, Meteo_Current** _MW_Ptr, Meteo_Hourly** _MF_Ptr)
{
  /* Dispose of Forecast */
  if (_MF_Ptr != NULL)
  {
    if (*_MF_Ptr != NULL)
    {
      /* if ((*_MF_Ptr)->weathers != NULL)
      {
        for (int i = 0; i < (*_MF_Ptr)->count; i++)
          meteo_dispose_ptr(NULL, &(*_MF_Ptr)->weathers[i], NULL);

        free((*_MF_Ptr)->weathers);
        (*_MF_Ptr)->weathers = NULL;
      }
      free(*_MF_Ptr);
      *_MF_Ptr = NULL; */
    }
    _MF_Ptr = NULL;
  }

  /* Dispose of Weather */
  if (_MW_Ptr != NULL)
  {
    if (*_MW_Ptr != NULL)
    {
      if ((*_MW_Ptr)->timezone_abbreviation != NULL)
      {
        printf("Disposing timezone_abbreviation\n");
        free((void*)(*_MW_Ptr)->timezone_abbreviation);
        (*_MW_Ptr)->timezone_abbreviation = NULL;
      }
      /* if ((*_MW_Ptr)->timestamp != NULL)
      {
        free((void*)(*_MW_Ptr)->timestamp);
        (*_MW_Ptr)->timestamp = NULL;
      } */
      if ((*_MW_Ptr)->temperature_2m_unit != NULL)
      {
        free((void*)(*_MW_Ptr)->temperature_2m_unit);
        (*_MW_Ptr)->temperature_2m_unit = NULL;
      }
      if ((*_MW_Ptr)->elevation_unit != NULL)
      {
        free((void*)(*_MW_Ptr)->elevation_unit);
        (*_MW_Ptr)->elevation_unit = NULL;
      }
      if ((*_MW_Ptr)->precipitation_unit != NULL)
      {
        free((void*)(*_MW_Ptr)->precipitation_unit);
        (*_MW_Ptr)->precipitation_unit = NULL;
      }
      if ((*_MW_Ptr)->wind_speed_10m_unit != NULL)
      {
        free((void*)(*_MW_Ptr)->wind_speed_10m_unit);
        (*_MW_Ptr)->wind_speed_10m_unit = NULL;
      }
      if ((*_MW_Ptr)->wind_direction_10m_unit != NULL)
      {
        free((void*)(*_MW_Ptr)->wind_direction_10m_unit);
        (*_MW_Ptr)->wind_direction_10m_unit = NULL;
      }

      free(*_MW_Ptr);
      *_MW_Ptr = NULL;
    }
    _MW_Ptr = NULL;
  }
}
