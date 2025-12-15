#include "api/weather_api.h"
#include "HTTPStatusCodes.h"
#include "error.h"
#include "scheduler.h"
#include "api/weather_parser.h"
#include <string.h>

/** ----------------------- INTERNAL DEFS ------------------------ */

/* Functions to be called on specific path from request */
int weather_api_handle_endpoint_weather_get(Weather_API* _API);
int weather_api_handle_endpoint_forecast_get(Weather_API* _API);
int weather_api_handle_endpoint_geo_get(Weather_API* _API);
int weather_api_handle_endpoint_not_found(Weather_API* _API);


/* REMEMBER TO CHANGE COUNT WHEN ADDING ENDPOINT! */
#define ENDPOINTS_COUNT 3

const Weather_API_Endpoint Endpoints[ENDPOINTS_COUNT] = {
  { 
    "/weather",
    HTTP_GET, 
    weather_api_handle_endpoint_weather_get, 
  },
  { 
    "/forecast", 
    HTTP_GET, 
    weather_api_handle_endpoint_not_found,
  },
  { 
    "/geo",
    HTTP_GET, 
    weather_api_handle_endpoint_geo_get, 
  },
};


void weather_api_taskwork(void* _context, uint64_t _montime);
WeatherApiState worktask_decide_endpoint(Weather_API* _API);
WeatherApiState worktask_validate_endpoint(Weather_API* _API);
WeatherApiState worktask_respond(Weather_API* _API);

int weather_api_on_parser_finish(void* _Context);

/** -------------------------------------------------------------- */

/** Heap init Weather_API struct and family */
int weather_api_init_ptr(Weather_API** _Weather_API_Ptr, HTTP_Request* _HTTP_Req, HTTP_Response* _HTTP_Res, on_api_finish _on_finish, void* _context)
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
  (*_Weather_API_Ptr)->on_api_finish = _on_finish;
  (*_Weather_API_Ptr)->context = _context;
  (*_Weather_API_Ptr)->state = WEATHER_API_INITIALIZING;

  (*_Weather_API_Ptr)->task = scheduler_create_task((*_Weather_API_Ptr), weather_api_taskwork);

  if (!(*_Weather_API_Ptr)->task) 
  {
    weather_api_dispose_ptr(_Weather_API_Ptr);
    return ERR_FATAL;
  }

  return SUCCESS;
}

int weather_api_on_parser_finish(void* _context)
{
  if (!_context) {
    return ERR_INVALID_ARG;
  }

  Weather_API* API = (Weather_API*)_context;

  printf("weather_api_on_parser_finish\n");
  API->state = WEATHER_API_RESPONDING;
  return SUCCESS;
}

/** ---------------------- ENDPOINT FUNCTIONS -------------------- */

int weather_api_handle_endpoint_geo_get(Weather_API* _API)
{
  if (_API->http_request->params_count < 1)
  {
    _API->http_response->status_code = 400;
    return ERR_INVALID_ARG;
  }

  char* query;
  int geos_count = 5; // default amount of geos (max) to get
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
      if (strcmp(Param->key, "count") == 0)
      {
        int c = 0;
        if (parse_string_to_int(Param->value, &c) == 0)
        {
          if (geos_count <= MAX_GEO_RESULTS)
            geos_count = c;
        }
      }
    }
  }

  if (query_found > 0)
  {
    Geos* geos;
    /* Init Location without weather or forecast*/
    if (geo_parser_init_ptr(&geos, geos_count, false, false) != 0)
    {
      perror("weather_parser_init_ptr");
      _API->http_response->status_code = 500;
      free(query);
      return ERR_INTERNAL;
    }

    if (geo_parser_get_geo_by_query(geos, query, &_API->http_response->body) != 0)
    {
      perror("geo_parser_get_geo_by_query");
      geo_parser_dispose_ptr(&geos);
      _API->http_response->status_code = 500;
      return ERR_INTERNAL;
    }
    geo_parser_dispose_ptr(&geos);
  }
  else
  {
    _API->http_response->status_code = 400;
    free(query);
    return ERR_INVALID_ARG;
  }

  _API->http_response->status_code = 200;

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
  float lat = 0, lon = 0;
  int lat_found = 0, lon_found = 0;
  /* int city_found = 0; */
  linked_list_foreach(_API->http_request->params, node)
  {
    HTTP_Key_Value* Param = (HTTP_Key_Value*)node->item;
    if (Param->key != NULL && Param->value != NULL)
    {
      if (strcmp(Param->key, "latitude") == 0 || strcmp(Param->key, "lat") == 0)
        lat_found += geo_parser_lat_lon(Param->value, &lat);

      if (strcmp(Param->key, "longitude") == 0 || strcmp(Param->key, "lon") == 0)
        lon_found += geo_parser_lat_lon(Param->value, &lon);

      /* if (strcmp(Param->key, "city") == 0 || strcmp(Param->key, "cityname") == 0)
        city_found += weather_parser_city() */

      // Potential to add "q=" param here to get weather from query
    }
  }

  /* Parse found query params and identify location */
  if (lat_found > 0 && lon_found > 0) // Could add an if else for city->name != NULL and let parser find coords for that city then
  {
    Weather* W; // fuck it, skip the geo struct here
    if (weather_parser_init_ptr() != 0)
    {
      perror("weather_parser_init_ptr");
      _API->http_response->status_code = 500;
      return -2;
    }

    if (weather_parser_get_weather_by_coords(W, lat, lon, OPEN_METEO_WEATHER, &_API->http_response->body) != 0)
    {
      perror("weather_parser_get_location_by_coords");
      weather_parser_dispose_ptr(&W, NULL);
      _API->http_response->status_code = 500;
      return -3;

    }
    weather_parser_dispose_ptr(&W, NULL);
  }
  else
  {
    _API->http_response->status_code = 400;
    return 0;
  }

  _API->http_response->status_code = 200;

  return SUCCESS;
}

/* --------------TASKWORK STATE FUNCTIONS-------------- */

WeatherApiState worktask_decide_endpoint(Weather_API* _API)
{
  const char* request_path = _API->http_request->path;

  int i;
  for (i = 0; i < ENDPOINTS_COUNT; i++) {
    const char* endpoint_path = Endpoints[i].path;
    if (strcmp(request_path, endpoint_path) == 0) {
      _API->endpoint_func = Endpoints[i].endpoint_func;
      return WEATHER_API_VALIDATING;
    }
  }

  _API->endpoint_func = weather_api_handle_endpoint_not_found;
  return WEATHER_API_RESPONDING;
}

WeatherApiState worktask_validate_endpoint(Weather_API* _API)
{
  if (_API->endpoint_func(_API) != SUCCESS)
  {
    
  }

  // Start parser taskwork with promise of callback when done
  // callback: weather_api_on_parser_finish


  return WEATHER_API_IDLING;
}

WeatherApiState worktask_respond(Weather_API* _API)
{
  // Build response etc, should

  _API->on_api_finish(_API->context);

  return WEATHER_API_DISPOSING;
}

void weather_api_taskwork(void* _context, uint64_t _montime)
{
  if (!_context) 
    return;
  
  Weather_API* API = (Weather_API*)_context;

  int result;
  switch (API->state)
  {
    case WEATHER_API_INITIALIZING:
    {
      printf("WEATHER_API_INITIALIZING\n");

    } break;

    case WEATHER_API_DECIDING_ENDPOINT:
    {
      printf("WEATHER_API_DECIDING_ENDPOINT\n");
      API->state = worktask_decide_endpoint(API);

    } break;

    case WEATHER_API_VALIDATING:
    {
      printf("WEATHER_API_VALIDATING\n");
      API->state = worktask_validate_endpoint(API);

    } break;

    case WEATHER_API_IDLING:
      break;

    case WEATHER_API_RESPONDING:
    {
      printf("WEATHER_API_RESPONDING\n");
      API->state = worktask_respond(API);

    } break;

    case WEATHER_API_DISPOSING:
    {
      printf("WEATHER_API_DISPOSING\n");

    } break;

    case WEATHER_API_ERROR:
    {

    } break;
  }
}


void weather_api_dispose_ptr(Weather_API** _API_Ptr)
{

  if (*_API_Ptr != NULL)
  {
    /* if ((*_API_Ptr)->geos != NULL)
      geo_parser_dispose_ptr(&(*_API_Ptr)->geos);
    if ((*_API_Ptr)->forecast != NULL)
      weather_parser_dispose_ptr(NULL, &(*_API_Ptr)->forecast); */

    scheduler_destroy_task((*_API_Ptr)->task);
    (*_API_Ptr)->task = NULL;

    free(*_API_Ptr);
    *_API_Ptr = NULL;
  }
  _API_Ptr = NULL;
}
