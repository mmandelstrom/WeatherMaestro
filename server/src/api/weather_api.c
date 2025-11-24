#include "../../include/api/weather_api.h"
#include <string.h>

/** ----------------------- INTERNAL DEFS ------------------------ */

/* Defines the API endpoints' paths and methods */
const Weather_API_Endpoint Endpoints[ENDPOINT_INVALID] = {
  { 
    ENDPOINT_WEATHER_GET, 
    HTTP_GET, 
    "/weather" 
  },
  { 
    ENDPOINT_FORECAST_GET, 
    HTTP_GET, 
    "/forecast" 
  },
  { 
    ENDPOINT_CITIES_LIST, 
    HTTP_GET, 
    "/cities" 
  },
  { 
    ENDPOINT_CITIES_COORDS, 
    HTTP_GET, 
    "/cities/geo" 
  },
  { 
    ENDPOINT_CITIES_ADD, // Should not be public, i.e should have an auth of some kind 
    HTTP_POST, 
    "/cities/add" 
  },
  { 
    ENDPOINT_CITIES_REMOVE, // Same as above
    HTTP_DELETE, 
    "/cities/remove" 
  },
};

WeatherAPIEndpoint weather_api_get_endpoint(const char* _request_path);
int weather_utils_parse_lat_lon(const char* _val, float* _target_coord);

int weather_api_handle_endpoint_weather_get(Weather_API* _Request);

/** -------------------------------------------------------------- */

/** Heap init Weather_API struct and family */
int weather_api_init_ptr(Weather_API** _Weather_API_Ptr, HTTP_Request* _HTTP_Req, HTTP_Response* _HTTP_Res)

{
  if (_Weather_API_Ptr == NULL)
    return -1;

  *_Weather_API_Ptr = malloc(sizeof(Weather_API));
  if (*_Weather_API_Ptr == NULL)
  {
    perror("malloc");
    return -2;
  }

  memset(*_Weather_API_Ptr, 0, sizeof(Weather_API));

  (*_Weather_API_Ptr)->http_request = _HTTP_Req;
  (*_Weather_API_Ptr)->http_response = _HTTP_Res;

  return 0;
}

/** ---------------------------- UTILS --------------------------- */

/** Return endpoint enum from path string */
WeatherAPIEndpoint weather_api_get_endpoint(const char* _request_path)
{
  int root_ep_len = strlen(API_ENDPOINT_ROOT);
  /* Check if req path is larger than root path and then if first part of it is identical to root path */
  if ((strlen(_request_path) > (size_t)root_ep_len) &&
      strncmp(_request_path, API_ENDPOINT_ROOT, root_ep_len) == 0)
  {
    for (int i = 0; i < (int)ENDPOINT_INVALID; i++)
    {
      if (strcmp(_request_path + root_ep_len, Endpoints[i].path) == 0)
        return Endpoints[i].endpoint;
    }
    // no endpoint for given path
    return ENDPOINT_INVALID;
  }
  else
  {
    // root api path not valid
    return ENDPOINT_INVALID;
  }
}

/** Takes a string and tries to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful, 0 if none parsed and -1 on error */
int weather_utils_parse_lat_lon(const char* _val, float* _target_coord)
{
  char coord[COORD_BUFFER_LENGTH];
  char* endptr; // for strtof

  int val_len = strlen(_val);
  errno = 0; // reset errno
  if (val_len > COORD_BUFFER_LENGTH)
  {
    strncpy(coord, _val, COORD_BUFFER_LENGTH);
    coord[COORD_BUFFER_LENGTH-1] = '\0';
    float coord_val = strtof(coord, &endptr);
    if (errno != 0 || endptr == coord || *endptr != '\0')
      return -1;

    *_target_coord = coord_val;
    return 1;
    
  } else {
    strncpy(coord, _val, val_len);
    coord[val_len] = '\0';

    float coord_val = strtof(coord, &endptr);
    if (errno != 0 || endptr == coord || *endptr != '\0')
      return -1;

    *_target_coord = coord_val;
    return 1;
  }

  return 0;
}

/** ---------------------- ENDPOINTS HANDLING -------------------- */

int weather_api_handle_endpoint(Weather_API* _API)
{
  WeatherAPIEndpoint endpoint = weather_api_get_endpoint(_API->http_request->path);

  int result;
  switch(endpoint)
  {
    case ENDPOINT_WEATHER_GET:
      {
        printf("ENDPOINT_WEATHER      \n");
        result = weather_api_handle_endpoint_weather_get(_API);

        printf("result: %i\n", result);

      } break;
    case ENDPOINT_FORECAST_GET:
      {
        printf("ENDPOINT_FORECAST_GET \n");
      } break;
    case ENDPOINT_CITIES_LIST:
      {
        printf("ENDPOINT_CITIES_LIST  \n");
      } break;
    case ENDPOINT_CITIES_COORDS:
      {
        printf("ENDPOINT_CITIES_COORDS\n");
      } break;
    case ENDPOINT_CITIES_ADD:
      {
        printf("ENDPOINT_CITIES_ADD   \n");
      } break;
    case ENDPOINT_CITIES_REMOVE:
      {
        printf("ENDPOINT_CITIES_REMOVE\n");
      } break;
    case ENDPOINT_INVALID:
      {
        printf("ENDPOINT_INVALID      \n");
      } break;
  }

  return 0;
}

int weather_api_handle_endpoint_weather_get(Weather_API* _API)
{
  if (_API->http_request->params_count < 2)
  {
    _API->http_response->status_code = 400;
    return -1;
  }

  City* New_City = malloc(sizeof(City));
  if (New_City == NULL)
  {
    _API->http_response->status_code = 500;
    perror("malloc");
    return -2;
  }

  _API->city = New_City;

  int lat_found = 0;
  int lon_found = 0;
  for (int i = 0; i < _API->http_request->params_count; i++)
  {
    yuarel_param* param = _API->http_request->params;
    if (param[i].key != NULL && param[i].val != NULL)
    {
      if (strcmp(param[i].key, "latitude") == 0)
        lat_found += weather_utils_parse_lat_lon(param[i].val, &_API->city->lat);

      if (strcmp(param[i].key, "longitude") == 0)
        lon_found += weather_utils_parse_lat_lon(param[i].val, &_API->city->lon);

      // Potential to add "city=" param here to get lat+lon in the same request
      // Also specific weather data, if not all is wanted
    }
  }
  if (lat_found > 0 && lon_found > 0)
  {
    /* int result = meteo_get_weather(_API->city->lat, _API->city->lon, ); */

    printf("lat: %f\n", _API->city->lat);
    printf("lon: %f\n", _API->city->lon);
  }
  printf("lat_found; %i, lon_found: %i\n", lat_found, lon_found);

  free(_API->city);
  _API->city = NULL;

  return 0;
}

void weather_api_dispose_ptr(Weather_API** _API_Ptr)
{

  if (*_API_Ptr != NULL)
  {
    free(*_API_Ptr);
    *_API_Ptr = NULL;
  }
  _API_Ptr = NULL;

}
