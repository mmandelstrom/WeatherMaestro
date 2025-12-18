#include "api/meteo.h"
#include "cJSON.h"
#include "http_parser.h"
#include <error.h>
#include <stdio.h>


/* ---------------------- Internal functions ----------------------- */

/* Only init the one you need, pass NULL to the others*/
int meteo_init_ptr(Meteo** _M_Ptr, Meteo_Geo** _MG_Ptr, Meteo_Current** _MW_Ptr, Meteo_Hourly** _MF_Ptr);

/* WEATHER/FORECAST */
int meteo_get_weather_json(Meteo* _M, float _lat, float _lon, bool _forecast);
int meteo_parse_json_weather_current(Meteo_Current* _MW, const char* _json);
int meteo_parse_json_weather_hourly(Meteo_Hourly* _MF, const char* _json);
void meteo_on_http_client_finish(void* _context, char** _response);
/* GEO */
// const char* meteo_get_geo_json(float _lat, float _lon);
// int meteo_parse_json_geo(Meteo_Geo* _MG, const char* _json);

void meteo_dispose_ptr(Meteo** _M_Ptr, Meteo_Geo** _MG_Ptr, Meteo_Current** _MW_Ptr, Meteo_Hourly** _MF_Ptr);

/* ----------------------------------------------------------------- */

int meteo_get_geo(Meteo** _M_Ptr, float _lat, float _lon, int _count, on_ext_api_finish _on_finish, void* _context)
{
  Meteo_Geo* Geo;
  meteo_init_ptr(_M_Ptr, &Geo, NULL, NULL);
  (*_M_Ptr)->geo = Geo;
  (*_M_Ptr)->on_finish = _on_finish;
  (*_M_Ptr)->context = _context;


  return SUCCESS;
}
int meteo_get_weather_current(Meteo** _M_Ptr, float _lat, float _lon, on_ext_api_finish _on_finish, void* _context)
{

  meteo_init_ptr(_M_Ptr, NULL, NULL, NULL);
  meteo_init_ptr(NULL, NULL, &(*_M_Ptr)->current, NULL);

  (*_M_Ptr)->on_finish = _on_finish;
  (*_M_Ptr)->context = _context;

  int result = meteo_get_weather_json(*_M_Ptr, _lat, _lon, false);

  return result;
}
int meteo_get_weather_hourly(Meteo** _M_Ptr, float _lat, float _lon, on_ext_api_finish _on_finish, void* _context)
{
  Meteo_Hourly* Hourly;
  meteo_init_ptr(_M_Ptr, NULL, NULL, &Hourly);
  (*_M_Ptr)->hourly = Hourly;

  (*_M_Ptr)->on_finish = _on_finish;
  (*_M_Ptr)->context = _context;

  int result = meteo_get_weather_json(*_M_Ptr, _lat, _lon, true);

  return result;
}

int meteo_init_ptr(Meteo** _M_Ptr, Meteo_Geo** _MG_Ptr, Meteo_Current** _MW_Ptr, Meteo_Hourly** _MF_Ptr)
{
  /* Module struct */
  if (_M_Ptr != NULL)
  {
    *_M_Ptr = calloc(1, sizeof(Meteo));
    if (*_M_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
  }

  /* Geo */
  if (_MG_Ptr != NULL)
  {
    *_MG_Ptr = calloc(1, sizeof(Meteo_Geo));
    if (*_MG_Ptr == NULL)
    {
      perror("malloc");
      meteo_dispose_ptr(_M_Ptr, _MG_Ptr, _MW_Ptr, _MF_Ptr);
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
      meteo_dispose_ptr(_M_Ptr, _MG_Ptr, _MW_Ptr, _MF_Ptr);
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
      meteo_dispose_ptr(_M_Ptr, _MG_Ptr, _MW_Ptr, _MF_Ptr);
      return ERR_NO_MEMORY;
    }
    (*_MW_Ptr)->values = realloc((*_MW_Ptr)->values, sizeof(Meteo_Weather_Values));
    if ((*_MW_Ptr)->values == NULL)
    {
      perror("malloc");
      meteo_dispose_ptr(_M_Ptr, _MG_Ptr, _MW_Ptr, _MF_Ptr);
      return ERR_NO_MEMORY;
    }
  }

  return 0;
}

void meteo_on_http_client_finish(void* _context, char** _response)
{
  Meteo* M = (Meteo*)_context;
  /* if (M->geo != NULL)
    meteo_parse_json_geo(M, const char *_json) */

  printf("In meteo response: %s", *_response);

  if (M->current != NULL)
    meteo_parse_json_weather_current(M->current, *_response);

  if (M->hourly != NULL)
    meteo_parse_json_weather_hourly(M->hourly, *_response);

  M->on_finish(M->context, M);

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
    return ERR_NO_MEMORY;
  }
  HTTPMethod method = HTTP_GET;

  if (http_client_initiate(c, (const char*)url, method, meteo_on_http_client_finish, _M, &_M->http_response) != SUCCESS) 
  {
    return ERR_INTERNAL;
  }

  return SUCCESS;
}

int meteo_parse_json_weather_current(Meteo_Current* _MW, const char* _json)
{
  cJSON* Json_Root = cJSON_Parse(_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"meteo json error %s\n", error_pointer);
    }
    return ERR_JSON_PARSE;
  }

  cJSON* Cur_Weather = cJSON_GetObjectItemCaseSensitive(Json_Root, "current");
  if (Cur_Weather == NULL){
    fprintf(stderr, "'current' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return ERR_JSON_OBJ_NOT_FOUND;
  }
  cJSON* Cur_Weather_Units = cJSON_GetObjectItemCaseSensitive(Json_Root, "current_units");
  if (Cur_Weather_Units == NULL){
    fprintf(stderr, "'current_units' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return ERR_JSON_OBJ_NOT_FOUND;
  }

  /* Parse data from json */
  memcpy(_MW->values->timestamp, json_get_string(Cur_Weather, "time"), 16);
  _MW->values->timestamp[16] = '\0';

  _MW->latitude                = json_get_double(Json_Root, "latitude");
  _MW->longitude               = json_get_double(Json_Root, "longitude");
  _MW->generationtime_ms       = json_get_double(Json_Root, "generationtime_ms");
  _MW->elevation               = json_get_double(Json_Root, "elevation");

  _MW->values->precipitation      = json_get_double(Cur_Weather, "precipitation");
  _MW->values->temperature_2m     = json_get_double(Cur_Weather, "temperature_2m");
  _MW->values->wind_speed_10m     = json_get_double(Cur_Weather, "wind_speed_10m");
  _MW->values->wind_direction_10m = json_get_int(Cur_Weather, "wind_direction_10m");
  _MW->values->is_day             = json_get_int(Cur_Weather, "is_day");
  _MW->values->weather_code       = json_get_int(Cur_Weather, "weather_code");

  _MW->utc_offset_seconds      = json_get_int(Cur_Weather, "utc_offset_seconds");
  _MW->interval                = json_get_int(Cur_Weather, "interval");

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
    return ERR_JSON_PARSE;
  }

  cJSON_Delete(Json_Root);

  printf("meteo parsed temperature: %f %s\n", _MW->values->temperature_2m, _MW->temperature_2m_unit);

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
    return ERR_JSON_PARSE;
  }

  cJSON* Hourly_Weather = cJSON_GetObjectItemCaseSensitive(Json_Root, "hourly");
  if (Hourly_Weather == NULL){
    fprintf(stderr, "'hourly' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return ERR_JSON_OBJ_NOT_FOUND;
  }
  cJSON* Hourly_Weather_Units = cJSON_GetObjectItemCaseSensitive(Json_Root, "hourly_units");
  if (Hourly_Weather_Units == NULL){
    fprintf(stderr, "'hourly_units' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return ERR_JSON_OBJ_NOT_FOUND;
  }
  
  /* Parse the the first array to get count*/
  cJSON* Time = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "time");
  int count = cJSON_GetArraySize(Time);
  
  _MF->values = realloc(_MF->values, ((size_t)count * sizeof(Meteo_Weather_Values)));
  if (_MF->values == NULL){
    perror("realloc");
    cJSON_Delete(Json_Root);
    return ERR_NO_MEMORY;
  }
  _MF->count = count;

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
    return ERR_JSON_OBJ_NOT_FOUND;
  }
  
  _MF->latitude  = json_get_double(Json_Root, "latitude");
  _MF->longitude = json_get_double(Json_Root, "longitude");

  _MF->utc_offset_seconds = json_get_int(Hourly_Weather, "utc_offset_seconds");
  _MF->interval           = json_get_int(Hourly_Weather, "interval");
  _MF->generationtime_ms  = json_get_double(Json_Root, "generationtime_ms");
  _MF->elevation          = json_get_double(Json_Root, "elevation");

  _MF->timezone_abbreviation   = strdup(json_get_string(Json_Root, "timezone_abbreviation"));
  _MF->temperature_2m_unit     = strdup(json_get_string(Hourly_Weather_Units, "temperature_2m"));
  _MF->elevation_unit          = strdup(json_get_string(Hourly_Weather_Units, "elevation"));
  _MF->precipitation_unit      = strdup(json_get_string(Hourly_Weather_Units, "precipitation"));
  _MF->wind_speed_10m_unit     = strdup(json_get_string(Hourly_Weather_Units, "wind_speed_10m"));
  _MF->wind_direction_10m_unit = strdup(json_get_string(Hourly_Weather_Units, "wind_direction_10m"));

  /* Heap allocations */
  if (_MF->timezone_abbreviation   == NULL ||
      _MF->temperature_2m_unit     == NULL ||
      _MF->elevation_unit          == NULL ||
      _MF->precipitation_unit      == NULL ||
      _MF->wind_speed_10m_unit     == NULL ||
      _MF->wind_direction_10m_unit == NULL)
  {
    fprintf(stderr, "One or more strings couldn't be parsed from meteo json\n");
    cJSON_Delete(Json_Root);
    return ERR_JSON_PARSE;
  }

  for (int i = 0; i < count; i++)
  {

    cJSON_GetArrayItem(Temperature  , i);
    cJSON_GetArrayItem(Windspeed    , i);
    cJSON_GetArrayItem(Winddirection, i);
    cJSON_GetArrayItem(Precipitation, i);
    cJSON_GetArrayItem(Weathercode  , i);
    cJSON_GetArrayItem(Isday        , i);

    memcpy(_MF->values[i].timestamp, json_get_string(Hourly_Weather, "time"), 16);

    _MF->values[i].timestamp[16] = '\0';
                               
    _MF->values[i].precipitation      = json_get_double(Hourly_Weather, "precipitation");
    _MF->values[i].temperature_2m     = json_get_double(Hourly_Weather, "temperature_2m");
    _MF->values[i].wind_speed_10m     = json_get_double(Hourly_Weather, "wind_speed_10m");
    _MF->values[i].wind_direction_10m = json_get_int(Hourly_Weather, "wind_direction_10m");
    _MF->values[i].is_day             = json_get_int(Hourly_Weather, "is_day");
    _MF->values[i].weather_code       = json_get_int(Hourly_Weather, "weather_code");

  }

  cJSON_Delete(Json_Root);

  return 0;
}

void meteo_dispose_ptr(Meteo** _M_Ptr, Meteo_Geo** _MG_Ptr, Meteo_Current** _MW_Ptr, Meteo_Hourly** _MF_Ptr)
{

  /* Dispose of Hourly */
  if (_MF_Ptr != NULL)
  {
    if (*_MF_Ptr != NULL)
    {
      if ((*_MF_Ptr)->values != NULL)
      {
        free((*_MF_Ptr)->values);
        (*_MF_Ptr)->values = NULL;
      }
      free(*_MF_Ptr);
      *_MF_Ptr = NULL;
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

  /* Dispose of Module */
  if (_M_Ptr != NULL)
  {
    if (*_M_Ptr != NULL)
    {
      if ((*_M_Ptr)->geo != NULL)
        meteo_dispose_ptr(NULL, &(*_M_Ptr)->geo, NULL, NULL);
      if ((*_M_Ptr)->current != NULL)
        meteo_dispose_ptr(NULL, NULL, &(*_M_Ptr)->current, NULL);
      if ((*_M_Ptr)->hourly != NULL)
        meteo_dispose_ptr(NULL, NULL, NULL, &(*_M_Ptr)->hourly);

      free(*_M_Ptr);
      *_M_Ptr = NULL;
    }
    _M_Ptr = NULL;
  }
}
