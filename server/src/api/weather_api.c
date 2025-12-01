#include "../../include/api/weather_api.h"
#include <string.h>

/** ----------------------- INTERNAL DEFS ------------------------ */

/* Defines the API endpoints' paths and methods */
const Weather_API_Endpoint Endpoints[ENDPOINT_INVALID] = {
  { 
    "/weather",
    HTTP_GET, 
    ENDPOINT_WEATHER_GET, 
  },
  { 
    "/forecast", 
    HTTP_GET, 
    ENDPOINT_FORECAST_GET, 
  },
  { 
    "/geo/list",
    HTTP_GET, 
    ENDPOINT_GEO_LIST, 
  },
  { 
    "/geo",
    HTTP_GET, 
    ENDPOINT_GEO_GET, 
  },
  { 
    "/cities/add",
    HTTP_POST, 
    ENDPOINT_GEO_ADD, // Should not be public, i.e should have an auth of some kind 
  },
  { 
    "/cities/remove",
    HTTP_DELETE, 
    ENDPOINT_GEO_REMOVE, // Same as cities_add
  },
};

WeatherAPIEndpoint weather_api_get_endpoint(const char* _request_path);

int weather_api_handle_endpoint_weather_get(Weather_API* _Request);
int weather_api_handle_endpoint_geo_get(Weather_API* _API);

/** -------------------------------------------------------------- */

/** Heap init Weather_API struct and family */
int weather_api_init_ptr(Weather_API** _Weather_API_Ptr, HTTP_Request* _HTTP_Req, HTTP_Response* _HTTP_Res)

{
  if (_Weather_API_Ptr == NULL) {
    return ERR_INVALID_ARG;
  }

  *_Weather_API_Ptr = malloc(sizeof(Weather_API));
  if (*_Weather_API_Ptr == NULL)
  {
    perror("malloc");
    return ERR_NO_MEMORY;
  }

  memset(*_Weather_API_Ptr, 0, sizeof(Weather_API));

  (*_Weather_API_Ptr)->http_request = _HTTP_Req;
  (*_Weather_API_Ptr)->http_response = _HTTP_Res;

  return SUCCESS;
}

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

      } break;
    case ENDPOINT_FORECAST_GET:
      {
        printf("ENDPOINT_FORECAST_GET \n");
        _API->http_response->status_code = 404; // replace with actual implementation
      } break;
    case ENDPOINT_GEO_LIST:
      {
        printf("ENDPOINT_GEO_LIST  \n");
        _API->http_response->status_code = 404; // replace with actual implementation
      } break;
    case ENDPOINT_GEO_GET:
      {
        printf("ENDPOINT_GEO_GET\n");
        result = weather_api_handle_endpoint_geo_get(_API); 
      } break;
    case ENDPOINT_GEO_ADD:
      {
        printf("ENDPOINT_GEO_ADD   \n");
        _API->http_response->status_code = 404; // replace with actual implementation
      } break;
    case ENDPOINT_GEO_REMOVE:
      {
        printf("ENDPOINT_CITIES_REMOVE\n");
        _API->http_response->status_code = 404; // replace with actual implementation
      } break;
    case ENDPOINT_INVALID:
      {
        printf("ENDPOINT_INVALID      \n");
        _API->http_response->status_code = 404;
      } break;
  }

  return SUCCESS;
}

int weather_api_handle_endpoint_geo_get(Weather_API* _API)
{
  if (_API->http_request->params_count < 1)
  {
    _API->http_response->status_code = 400;
    return ERR_INVALID_ARG;
  }

  /* Init Location without weather or forecast*/
  if (weather_parser_init_ptr(&_API->location, false, false) != 0)
  {
    perror("weather_parser_init_ptr");
    _API->http_response->status_code = 500;
    return ERR_INTERNAL;
  }

  char* query;
  int query_found = 0;
  linked_list_foreach(_API->http_request->params, node)
  {
    HTTP_Key_Value* Param = (HTTP_Key_Value*)node->item;
    if (Param->key != NULL && Param->value != NULL)
    {
      if ((strcmp(Param->key, "q") == 0 || strcmp(Param->key, "city") == 0) && query_found < 1)
      {
        query = strdup(Param->value);
        query_found++;
      }
    }
  }

  if (query_found > 0)
  {
    if (weather_parser_get_location_by_query(_API->location, query) != 0)
    {
      perror("weather_parser_get_location_by_coords");
      free(query);
      _API->http_response->status_code = 500;
      return ERR_INTERNAL;
    }
  }
  else
  {
    _API->http_response->status_code = 400;
    return ERR_INVALID_ARG;
  }

  free(query);

  char* json_response = weather_parser_build_json_location(_API->location);

  _API->http_response->body = json_response;
  _API->http_response->status_code = 200;

  weather_parser_dispose_ptr(&_API->location, NULL, NULL);
  _API->location = NULL;

  return SUCCESS;
}

int weather_api_handle_endpoint_weather_get(Weather_API* _API)
{
  if (_API->http_request->params_count < 1)
  {
    _API->http_response->status_code = 400;
    return ERR_INVALID_ARG;
  }

  /* Find and validate latitude and longitude params */
  float lat, lon = 0;
  int lat_found, lon_found = 0;
  int city_found = 0;
  linked_list_foreach(_API->http_request->params, node)
  {
    HTTP_Key_Value* Param = (HTTP_Key_Value*)node->item;
    if (Param->key != NULL && Param->value != NULL)
    {
      if (strcmp(Param->key, "latitude") == 0 || strcmp(Param->key, "lat") == 0)
        lat_found += weather_parser_lat_lon(Param->value, &lat);

      if (strcmp(Param->key, "longitude") == 0 || strcmp(Param->key, "lon") == 0)
        lon_found += weather_parser_lat_lon(Param->value, &lon);

      /* if (strcmp(Param->key, "city") == 0 || strcmp(Param->key, "cityname") == 0)
        city_found += weather_parser_city() */

      // Potential to add "city=" param here to get lat+lon in the same request
      // Then we should have Location cache as well to look for previously searched cities
    }
  }

  /* Parse found query params and identify location */
  if ((lat_found > 0 && lon_found > 0)) // Could add an if else for city->name != NULL and let parser find coords for that city then
  {
    if (weather_parser_init_ptr(&_API->location, true, false) != 0)
    {
      perror("weather_parser_init_ptr");
      _API->http_response->status_code = 500;
      return -2;
    }
    printf("_Location: %p, _Location->weather: %p\n", _API->location, _API->location->weather); 

    if (weather_parser_get_location_by_coords(_API->location, lat, lon) != 0)
    {
      perror("weather_parser_get_location_by_coords");
      _API->http_response->status_code = 500;
      return -3;

    }
  }
  else if (city_found > 0)
  {
    if (weather_parser_init_ptr(&_API->location, true, false) != 0)
    {
      perror("weather_parser_init_ptr");
      _API->http_response->status_code = 500;
      return ERR_NO_MEMORY;
    }

    /* if (weather_parser_get_location_by_names(_API->location, Http_Data* _Query_Params) != 0)
    {
      perror("weather_parser_get_location_by_coords");
      _API->http_response->status_code = 500;
      return -3;

    } */

  }
  else
  {
    _API->http_response->status_code = 400;
    weather_parser_dispose_ptr(&_API->location, &_API->location->weather, NULL);
    return 0;
  }

  /* Build Location Weather from external API response */
  if (weather_parser_get_weather(_API->location, false, OPEN_METEO) != 0)
  {
    perror("weather_parser_get_weather");
    _API->http_response->status_code = 500;
    return -4;
  }

  char* json_response = weather_parser_build_json_weather(_API->location->weather);

  _API->http_response->body = json_response;
  _API->http_response->status_code = 200;

  weather_parser_dispose_ptr(&_API->location, NULL, NULL);
  _API->location = NULL;

  return SUCCESS;
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
