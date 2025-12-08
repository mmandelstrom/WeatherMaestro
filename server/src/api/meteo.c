#include "api/meteo.h"
#include <error.h>


/* ---------------------- Internal functions ----------------------- */

/* WEATHER/FORECAST */
const char* meteo_get_weather_json(float _lat, float _lon, bool _forecast);
int meteo_parse_json_weather_current(Meteo_Weather* _MW, const char* _json);
int meteo_parse_json_weather_hourly(Meteo_Forecast* _MF, const char* _json);

/* GEO */
// const char* meteo_get_geo_json(float _lat, float _lon);
// int meteo_parse_json_geo(Meteo_Geo* _MG, const char* _json);

/* ----------------------------------------------------------------- */

int meteo_init_ptr(Meteo_Geo** _MG_Ptr, Meteo_Weather** _MW_Ptr, Meteo_Forecast** _MF_Ptr)
{

  /* Geo */
  if (_MG_Ptr != NULL)
  {
    *_MG_Ptr = malloc(sizeof(Meteo_Geo));
    if (*_MG_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_MG_Ptr, 0, sizeof(Meteo_Geo));
  }

  /* Forecast */
  if (_MF_Ptr != NULL)
  {
    *_MF_Ptr = malloc(sizeof(Meteo_Forecast));
    if (*_MF_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_MF_Ptr, 0, sizeof(Meteo_Forecast));
  }

  /* Weather */
  if (_MW_Ptr != NULL)
  {
    *_MW_Ptr = malloc(sizeof(Meteo_Weather));
    if (*_MW_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_MW_Ptr, 0, sizeof(Meteo_Weather));
  }


  return 0;
}

int meteo_get_weather(Meteo_Weather* _MW, float _lat, float _lon)
{
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

  /* Parse meteo json to Meteo_Weather struct */
  int result = meteo_parse_json_weather_current(_MW, meteo_json);
  if (result != 0)
  {
    perror("meteo_parse_json");
    free((void*)meteo_json);
    return result;
  }
  free((void*)meteo_json);

  return 0;
}

int meteo_get_forecast(Meteo_Forecast* _MF, float _lat, float _lon)
{
  const char* meteo_json = meteo_get_weather_json(_lat, _lon, true);
  if (meteo_json == NULL)
  {
    perror("meteo_get_weather_json");
    return -1;
  }

  int result = meteo_parse_json_weather_forecast(_MF, meteo_json);
  if (result != 0)
  {
    perror("meteo_parse_json");
    free((void*)meteo_json);
    return result;
  }
  free((void*)meteo_json);

  return SUCCESS;
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
  
	// Curl_Data C_Data;
	// if (curl_init(&C_Data) != 0)
	// 	return NULL;
	//
	//  printf("\n--- CALLING METEO RESPONSE --- \n\n");
	//
	// int result = curl_get_response(&C_Data, url);
	// if (result != 0)
	// {
	// 	perror("curl_get_response");
	// 	curl_dispose(&C_Data);
	// 	return NULL;
	// }
	//
  HTTP_Client* c = calloc(1, sizeof(HTTP_Client));
  if (!c) {
    return NULL;
  }
  HTTPMethod method = 1;

  int http_client_initiate(HTTP_Client* _Client, const char* _URL, HTTPMethod _method);

  if (http_client_initiate(c, (const char*)url, method) != SUCCESS) {
    return NULL;
  }


  // char* response = malloc(C_Data.size + 1);
  // if (response == NULL)
  // {
  //   perror("malloc");
  // curl_dispose(&C_Data);
  // return NULL;
  // }
  //
  // memcpy(response, C_Data.addr, C_Data.size);
  // response[C_Data.size] = '\0';
  // curl_dispose(&C_Data);
  //
  // printf("===== Meteo Response JSON =====\n\n%s\n\n", response);

  return NULL;
}

int meteo_parse_weather_json(Meteo_Weather* _MW, const char* _json)
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

int meteo_parse_json_weather_hourly(Meteo_Forecast* _MF, const char* _json)
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
  cJSON* Hourly_Weather_Units = cJSON_GetObjectItemCaseSensitive(Json_Root, "current_units");
  if (Hourly_Weather_Units == NULL){
    fprintf(stderr, "'hourly_units' section missing in meteo json\n");
    cJSON_Delete(Json_Root);
    return -3;
  }
  
  /* Parse the the first array to get count*/
  cJSON* Time = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "time");
  int count = cJSON_GetArraySize(Time);
  _MF->count = count;
  
  _MF->weathers = realloc(_MF->weathers, count * sizeof(Meteo_Weather));
  if (!_MF->weathers)
  {
    perror("realloc");
    cJSON_Delete(Json_Root);
    return -4;
  }

  cJSON* Temperature = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "temperature_2m");
  cJSON* Windspeed = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "wind_speed_10m");
  cJSON* Winddirection = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "wind_direction_10m");
  cJSON* Precipitation = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "precipitation");
  cJSON* Weathercode = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "weather_code");
  cJSON* Isday = cJSON_GetObjectItemCaseSensitive(Hourly_Weather, "is_day");

  for (int i = 0; i < count; i++)
  {

    /* Parse data from json */
    memcpy(_MF->weathers[i]->timestamp, json_get_string(Hourly_Weather, "time"), 16);
    _MF->weathers[i]->timestamp[16] = '\0';

    _MF->weathers[i]->latitude                = json_get_double(Json_Root, "latitude");
    _MF->weathers[i]->longitude               = json_get_double(Json_Root, "longitude");
    _MF->weathers[i]->generationtime_ms       = json_get_double(Json_Root, "generationtime_ms");
    _MF->weathers[i]->elevation               = json_get_double(Json_Root, "elevation");

    _MF->weathers[i]->precipitation           = json_get_double(Hourly_Weather, "precipitation");
    _MF->weathers[i]->temperature_2m          = json_get_double(Hourly_Weather, "temperature_2m");
    _MF->weathers[i]->wind_speed_10m          = json_get_double(Hourly_Weather, "wind_speed_10m");

    _MF->weathers[i]->wind_direction_10m      = json_get_int(Hourly_Weather, "wind_direction_10m");
    _MF->weathers[i]->utc_offset_seconds      = json_get_int(Hourly_Weather, "utc_offset_seconds");
    _MF->weathers[i]->interval                = json_get_int(Hourly_Weather, "interval");
    _MF->weathers[i]->is_day                  = json_get_int(Hourly_Weather, "is_day");
    _MF->weathers[i]->weather_code            = json_get_int(Hourly_Weather, "weather_code");

    /* Heap allocations */
    _MF->weathers[i]->timezone_abbreviation   = strdup(json_get_string(Json_Root, "timezone_abbreviation"));
    _MF->weathers[i]->temperature_2m_unit     = strdup(json_get_string(Hourly_Weather_Units, "temperature_2m"));
    _MF->weathers[i]->elevation_unit          = strdup(json_get_string(Hourly_Weather_Units, "elevation"));
    _MF->weathers[i]->precipitation_unit      = strdup(json_get_string(Hourly_Weather_Units, "precipitation"));
    _MF->weathers[i]->wind_speed_10m_unit     = strdup(json_get_string(Hourly_Weather_Units, "wind_speed_10m"));
    _MF->weathers[i]->wind_direction_10m_unit = strdup(json_get_string(Hourly_Weather_Units, "wind_direction_10m"));
    if (_MF->weathers[i]->timezone_abbreviation   == NULL ||
        _MF->weathers[i]->temperature_2m_unit     == NULL ||
        _MF->weathers[i]->elevation_unit          == NULL ||
        _MF->weathers[i]->precipitation_unit      == NULL ||
        _MF->weathers[i]->wind_speed_10m_unit     == NULL ||
        _MF->weathers[i]->wind_direction_10m_unit == NULL)
    {
      fprintf(stderr, "One or more strings couldn't be parsed from meteo json\n");
      cJSON_Delete(Json_Root);
      return -4;
    }
    printf("meteo parsed temperature: %f %s\n", _MF->weathers[i]->temperature_2m, _MF->weathers[i]->temperature_2m_unit);
  }
  

  cJSON_Delete(Json_Root);


  return 0;
}

void meteo_dispose_ptr(Meteo_Forecast** _MF_Ptr, Meteo_Weather** _MW_Ptr)
{
  /* Dispose of Forecast */
  if (_MF_Ptr != NULL)
  {
    if (*_MF_Ptr != NULL)
    {
      if ((*_MF_Ptr)->weathers != NULL)
      {
        for (int i = 0; i < (*_MF_Ptr)->count; i++)
          meteo_dispose_ptr(NULL, &(*_MF_Ptr)->weathers[i]);

        free((*_MF_Ptr)->weathers);
        (*_MF_Ptr)->weathers = NULL;
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
}
