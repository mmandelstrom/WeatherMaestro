#include "../../include/api/meteo.h"

int meteo_init_ptr(Meteo_Weather** _M_W_Ptr)
{

  if (_M_W_Ptr == NULL)
    return -1;

  *_M_W_Ptr = malloc(sizeof(Meteo_Weather));
  if (*_M_W_Ptr == NULL)
  {
    perror("malloc");
    return -2;
  }

  memset(*_M_W_Ptr, 0, sizeof(Meteo_Weather));

  return 0;
}

const char* meteo_get_weather_json(float _lat, float _lon)
{
  const char* mock = "{\"latitude\":56.768044,\"longitude\":12.119537,\"generationtime_ms\":138.96715641021729,\"utc_offset_seconds\":0,\"timezone\":\"GMT\",\"timezone_abbreviation\":\"GMT\",\"elevation\":0,\"current_units\":{\"time\":\"iso8601\",\"interval\":\"seconds\",\"temperature_2m\":\"°C\",\"relative_humidity_2m\":\"%\",\"apparent_temperature\":\"°C\",\"is_day\":\"\",\"precipitation\":\"mm\",\"rain\":\"mm\",\"showers\":\"mm\",\"snowfall\":\"cm\",\"weather_code\":\"wmo code\",\"cloud_cover\":\"%\",\"pressure_msl\":\"hPa\",\"surface_pressure\":\"hPa\",\"wind_speed_10m\":\"km/h\",\"wind_direction_10m\":\"°\",\"wind_gusts_10m\":\"km/h\"},\"current\":{\"time\":\"2025-11-24T14:00\",\"interval\":900,\"temperature_2m\":2.7,\"relative_humidity_2m\":69,\"apparent_temperature\":-5.2,\"is_day\":1,\"precipitation\":0,\"rain\":0,\"showers\":0,\"snowfall\":0,\"weather_code\":3,\"cloud_cover\":100,\"pressure_msl\":997.1,\"surface_pressure\":997.1,\"wind_speed_10m\":37.1,\"wind_direction_10m\":103,\"wind_gusts_10m\":48.6}}";


  return mock;
}

int meteo_parse_json(const char* _json, Meteo_Weather* _Meteo_Weather)
{
  Meteo_Weather MW = {0}; /*MWD Struct to hold all specific data fields*/

  cJSON* Json_Root = cJSON_Parse(_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"JSON error %s\n", error_pointer);
    }
    return -1;
  }

  cJSON* current_weather = cJSON_GetObjectItemCaseSensitive(Json_Root, "current");
  if (current_weather == NULL){
    fprintf(stderr, "current_weather section missing in JSON\n");
    cJSON_Delete(Json_Root);
    return -1;
  }
  cJSON* current_weather_units = cJSON_GetObjectItemCaseSensitive(Json_Root, "current_units");

  
  /*Populates MWD Struct with all data fields*/
  MW.latitude = (float)parsedata_get_double(current_weather, "latitude");
  MW.longitude = (float)parsedata_get_double(current_weather, "longitude");

  MW.generationtime_ms = parsedata_get_double(current_weather, "generationtime_ms");
  MW.utc_offset_seconds = parsedata_get_int(current_weather, "utc_offset_seconds");
  strcpy(MW.timezone_abbreviation, parsedata_get_string(current_weather, "timezone_abbreviation"));

  MW.elevation = parsedata_get_double(current_weather, "elevation");
  strcpy(MW.timestamp, parsedata_get_string(current_weather, "time"));
  MW.interval = parsedata_get_int(current_weather, "interval");
  MW.temperature = parsedata_get_double(current_weather, "temperature");
  MW.windspeed = parsedata_get_double(current_weather, "windspeed");
  MW.winddirection = parsedata_get_int(current_weather, "winddirection");
  MW.is_day = parsedata_get_int(current_weather, "is_day");
  MW.weather_code = parsedata_get_int(current_weather, "weathercode");


  cJSON_Delete(current_weather);

  return 0;
}
