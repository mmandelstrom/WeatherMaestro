#include "api/geo_parser.h"
#include "error.h"
#include "file_utils.h"
#include "scheduler.h"
#include "weather_parser.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/* Pre-hashed cache filename definitions */
#define CACHE_FILENAME_GEO_COORDS "lat%.3f_lon%.3f_geo"
#define CACHE_FILENAME_GEO_QUERY "%s_geo"

/* ---------------------- Internal functions ----------------------- */

/** Returns the full filepath for a cache file
 * If _Geo->query is not NULL it will return a cache name by that query string, 
 * else look for lat and lon values 
 * Writes to heap, free'd by caller */
char* geo_parser_get_cache_filepath(Geo_Parser* _Parser);

/* int geo_parser_get_geo_from_cache(Geo* _Geo, const char* _filepath); */

int geo_parser_parse_bigdatacloud_geo(Geo* _Geo, Bigdatacloud_Geo* _BDC_Geo);
int geo_parser_parse_nominatim_geo(Geo* _Geo, Nominatim_Geo* _NOM_Geo, int _geo_count);

int geo_parser_get_geo_from_api_by_coords(Geo* _Geo, ExternalGeoAPI _ExtAPI);
int geo_parser_get_geo_from_api_by_query(Geo* _Geo, ExternalGeoAPI ExtAPI);

/** Builds and returns the parsed json output from Geo struct
 * Also saves cache file */
char* geo_parser_build_json(Geo* _Geo);


/* TASKWORK */
void geo_parser_on_ext_api_finish(void* _context, void* _ext_api);

void geo_parser_taskwork(void* _context, uint64_t _montime);
GeoParserState geo_parser_worktask_check_cache(Geo_Parser* _GP);
GeoParserState geo_parser_worktask_call_ext_api(Geo_Parser* _GP);
GeoParserState geo_parser_worktask_parse_api_response(Geo_Parser* _GP);
GeoParserState geo_parser_worktask_respond(Geo_Parser* _GP);

/* ----------------------------------------------------------------- */


int geo_parser_init_ptr(Geo_Parser** _Parser_Ptr, void* _context, bool _use_query, const char* _query, float _lat, float _lon)
{
  if (_Parser_Ptr == NULL) {
    return ERR_INVALID_ARG;
  } 

  *_Parser_Ptr = malloc(sizeof(Geo_Parser));
  if (*_Parser_Ptr) {
    perror("malloc");
    return ERR_NO_MEMORY;
  }

  Geo* geo = malloc(sizeof(Geo));
  if (geo == NULL) {
    perror("malloc");
    return ERR_NO_MEMORY;
  }

  (*_Parser_Ptr)->geo = geo;
  (*_Parser_Ptr)->query = _query;
  (*_Parser_Ptr)->latitude = _lat;
  (*_Parser_Ptr)->longitude = _lon;
  (*_Parser_Ptr)->use_query = _use_query;
  (*_Parser_Ptr)->task = scheduler_create_task((*_Parser_Ptr), geo_parser_taskwork);

  if (!(*_Parser_Ptr)->task) {
    geo_parser_dispose_ptr(_Parser_Ptr);
    return ERR_INTERNAL;
  }

  (*_Parser_Ptr)->state = GEO_PARSER_CHECKING_CACHE;

  return SUCCESS;
}


GeoParserState geo_parser_worktask_check_cache(Geo_Parser* _Parser) {
  if (_Parser == NULL) {
    return GEO_PARSER_ERROR;
  }

  _Parser->geo->cache_path = geo_parser_get_cache_filepath(_Parser);

  printf("geo cache path: %s\n", _Parser->geo->cache_path);

  if (file_exists(_Parser->geo->cache_path)) {
    printf("Getting geo from cache\n");
    _Parser->json_output = read_file_to_string(_Parser->geo->cache_path);
    return GEO_PARSER_RESPONDING;
  } 
  
  return GEO_PARSER_CALLING_EXT_API;
}

GeoParserState geo_parser_worktask_call_ext_api(Geo_Parser* _Parser) {
  if (_Parser == NULL) {
    return GEO_PARSER_ERROR;
  }
  int result;

  if (_Parser->use_query == true) {
    result = geo_parser_get_geo_by_query(_Parser);
    if (result != SUCCESS) {
      return GEO_PARSER_ERROR;
    }
    return GEO_PARSER_IDLING;

  } else {

    result = geo_parser_get_geo_by_coords(_Parser);
    if (result != SUCCESS) {
      return GEO_PARSER_ERROR;
    }

    return GEO_PARSER_IDLING;
  }

  return GEO_PARSER_ERROR;
}


/** Builds Geo structs using cache or external API by query
 * If the same query has been made before then there is a cache */
int geo_parser_get_geo_by_query(Geo_Parser* _Parser)
{
  /*TODO: coordinate error codes so weather_api can decide whether the error was because of an internal server error or because the client request was faulty */

  int result;

  printf("Getting geo from API\n");
  result = geo_parser_get_geo_from_api_by_query(_Parser->geo, NOMINATIM_OSM_GEO);
  if (result != SUCCESS)
  {
    perror("geo_parser_get_geo_from_api_by_query");
    return result;
  }
  _Parser->json_output = geo_parser_build_json(_Parser->geo);
  
  printf("Json output: \n%s\n", _Parser->json_output);

  return SUCCESS;
}



/** NOT IMPLEMENTED First looks in cache by lat and lon, if none found search via API */
int geo_parser_get_geo_by_coords(Geo_Parser* _Parser)
{
  /*TODO: coordinate error codes so weather_api can decide whether the error was because of an internal server error or because the client request was faulty */
  
  ExternalGeoAPI ExtAPI = BIGDATACLOUD_GEO;
  int result;

  _Parser->geo->cache_path = geo_parser_get_cache_filepath(_Parser);

  printf("Getting geo from API\n");
  result = geo_parser_get_geo_from_api_by_coords(_Parser->geo, BIGDATACLOUD_GEO);
  if (result != 0)
  {
    perror("geo_parser_get_geo_from_api_by_coords");
    return result;
  }

  _Parser->json_output = geo_parser_build_json(_Parser->geo);

  return SUCCESS;
}

int geo_parser_get_geo_from_api_by_query(Geo* _Geo, ExternalGeoAPI _ExtAPI)
{
  int result;

  if (!_Geo || !_Geo->query)
    return -1;

  if (_ExtAPI == NOMINATIM_OSM_GEO)
  {
    Nominatim_Geo* NOM_Geo;
    result = nominatim_init_ptr(&NOM_Geo);
    if (result != 0)
    {
      perror("nominatim_init_ptr");
      return -2;
    }

    /* Get fresh Nominatim_Geo struct from API */
    int nom_geo_count = 0;
    result = nominatim_get_geo_by_query(&NOM_Geo, &nom_geo_count, _Geo->query);
    if (result != 0)
    {
      perror("nominatim_get_weather");
      nominatim_dispose_ptr(&NOM_Geo, nom_geo_count);
      return -3;
    }


    for (int i = 0; i < nom_geo_count; i++)
      printf("NOM_Geo[%i] lat: %lf lon: %lf\n", i, NOM_Geo[i].lat, NOM_Geo[i].lon);

    result = geo_parser_parse_nominatim_geo(_Geo, NOM_Geo, nom_geo_count);
    if (result != 0)
    {
      perror("weather_parser_parse_nominatim_weather");
      nominatim_dispose_ptr(&NOM_Geo, nom_geo_count);
      return -4;
    }
    nominatim_dispose_ptr(&NOM_Geo, nom_geo_count);

  }
  else 
  {
    // 501 not implemented
    return -5;
  }

  return 0;
}
int geo_parser_get_geo_from_api_by_coords(Geo* _Geo, ExternalGeoAPI _ExtAPI)
{
  int result;

  /* NOT IMPLEMENTED YET*/
  if (_ExtAPI == NOMINATIM_OSM_GEO)
  {
    Nominatim_Geo* NOM_Geo;
    result = nominatim_init_ptr(&NOM_Geo);
    if (result != 0)
    {
      perror("nominatim_init_ptr");
      return -2;
    }

    /*TODO: FInish get geo by coord*/
    /* Get fresh struct from external API */
    int nom_geo_count = 0;
    /* result = nominatim_get_geo_by_coords(NOM_Geo, &nom_geo_count, _Geo->lat, _Geo->lon); */
    if (result != 0)
    {
      perror("nominatim_get_geo_by_coords");
      nominatim_dispose_ptr(&NOM_Geo, nom_geo_count);
      return -3;
    }
    
    printf("---Nominatim Geo---\ncityname: %s | country %s\n lat: %f, lon: %f\n\n", NOM_Geo->city, NOM_Geo->country, NOM_Geo->lat, NOM_Geo->lon);

    result = geo_parser_parse_nominatim_geo(_Geo, NOM_Geo, nom_geo_count);
    if (result != 0)
    {
      perror("geo_parser_parse_nominatim_geo");
      nominatim_dispose_ptr(&NOM_Geo, nom_geo_count);
      return -4;
    }
    nominatim_dispose_ptr(&NOM_Geo, nom_geo_count);

  }
  else if (_ExtAPI == BIGDATACLOUD_GEO)
  {
    Bigdatacloud_Geo* BDC_Geo;
    result = bigdatacloud_init_ptr(&BDC_Geo);
    if (result != 0)
    {
      perror("bigdatacloud_init_ptr");
      return -2;
    }

    result = bigdatacloud_get_geo_by_coords(BDC_Geo, _Geo->values[0]->lat, _Geo->values[0]->lon);
    if (result != 0)
    {
      perror("bigdatacloud_get_geo_by_coords");
      bigdatacloud_dispose_ptr(&BDC_Geo);
      return -3;
    }

    printf("---Bigdatacloud Geo---\ncityname: %s | country %s\n lat: %f, lon: %f\n\n", BDC_Geo->city, BDC_Geo->country_name, BDC_Geo->latitude, BDC_Geo->longitude);

    result = geo_parser_parse_bigdatacloud_geo(_Geo->values[0], BDC_Geo);
    if (result != 0)
    {
      perror("geo_parser_parse_bigdatacloud_geo");
      bigdatacloud_dispose_ptr(&BDC_Geo);
      return -4;
    }
    bigdatacloud_dispose_ptr(&BDC_Geo);
  }

  return -1;
}

int geo_parser_parse_nominatim_geo(Geo* _Geo, Nominatim_Geo* _NOM_Geo, int _geo_count)
{
  /* Reallocate more memory for Geo if there are more than one Nom_Geo structs */

  for (int i = 0; i < _geo_count; i++)
  {
    if (i < _Geo->count) // Make sure we don't try to parse more than we have allocated
    {
      _Geo->values[i]->lat = _NOM_Geo[i].lat;
      _Geo->values[i]->lon = _NOM_Geo[i].lon;

      /* Assign string members */
      memcpy(_Geo->values[i]->country_code, _NOM_Geo[i].country_code, 2);
      _Geo->values[i]->country_code[2] = '\0';

      if(_NOM_Geo[i].country       != NULL)
        _Geo->values[i]->country  = strdup(_NOM_Geo[i].country);
      if(_NOM_Geo[i].county        != NULL)
        _Geo->values[i]->county   = strdup(_NOM_Geo[i].county);
      if(_NOM_Geo[i].city          != NULL)
        _Geo->values[i]->city     = strdup(_NOM_Geo[i].city);
      if(_NOM_Geo[i].postcode      != NULL)
        _Geo->values[i]->postcode = strdup(_NOM_Geo[i].postcode);
      if(_NOM_Geo[i].road          != NULL)
        _Geo->values[i]->street   = strdup(_NOM_Geo[i].road);
      if (_NOM_Geo[i].house_number != NULL)
        _Geo->values[i]->house_number = strdup(_NOM_Geo[i].house_number);

      printf("_NOM_Geo[i].latitude: %f", _NOM_Geo[i].lat);
      printf("_Geo->values[i]->latitude: %f", _Geo->values[i]->lat);
    }
  }

  return 0;
}

int geo_parser_parse_bigdatacloud_geo(Geo* _Geo, Bigdatacloud_Geo* _BDC_Geo)
{
  _Geo->lat = _BDC_Geo->latitude; // should these be updated here?
  _Geo->lon = _BDC_Geo->longitude;

  /* Assign string members */
  memcpy(_Geo->country_code, _BDC_Geo->country_code, 2);
  _Geo->country_code[2] = '\0';

  _Geo->city         = strdup(_BDC_Geo->city);
  _Geo->country      = strdup(_BDC_Geo->country_name);
  _Geo->locality     = strdup(_BDC_Geo->locality);

  if (_Geo->locality == NULL ||
      _Geo->country  == NULL || 
      _Geo->city     == NULL)
  {
    perror("Failed to duplicate bigdatacloud strings");
    return -1;
  }

  printf("_BDC_Geo->latitude: %f", _BDC_Geo->latitude);
  printf("_Geo->latitude: %f", _Geo->lat);

  return 0;
}

/** Pre-reqs either _Geo->query set or _Geo->values[0] lat and lon set */
char* geo_parser_get_cache_filepath(Geo_Parser* _Parser)
{
  char* file_ext = ".json";
  char filename_buf[256];

  int filename_len; 
  int file_ext_len = strlen(file_ext);
  int cache_dir_len = strlen(CACHE_DIR);
  int filepath_len; // full path

  if (_Parser->use_query == true) // use query for filename
  {
    int query_len = strlen(_Parser->geo->query);
    const char* hashed_filename = MD5_HashToString(_Parser->geo->query, query_len);

    filepath_len = cache_dir_len + strlen(hashed_filename) + query_len;
    char* full_filepath = malloc(filepath_len + 1);
    if (full_filepath == NULL)
    {
      perror("malloc");
      return NULL;
    }
    snprintf(full_filepath, filepath_len, "%s%s%s",
        CACHE_DIR, hashed_filename, file_ext);

    return full_filepath;
  }
  else if (_Parser->use_query == false) // use lat+lon for filename via CACHE_FILENAME_LOCATION template
  {
    filename_len = snprintf(filename_buf, 256,
          CACHE_FILENAME_GEO_COORDS,
          _Parser->geo->values[0]->lat,
          _Parser->geo->values[0]->lon);
    const char* hashed_filename = MD5_HashToString(filename_buf, filename_len);

    filepath_len = cache_dir_len + strlen(hashed_filename) + filename_len;
    char* full_filepath = malloc(filepath_len + 1);
    if (full_filepath == NULL)
    {
      perror("malloc");
      return NULL;
    }
    snprintf(full_filepath, filepath_len, "%s%s%s",
        CACHE_DIR, hashed_filename, file_ext);

    return full_filepath;
  }

  return NULL;
}

/** Builds Geo struct from cache path
 * This was obviously not needed, we can just read the cache file directly to the output pointer... But I guess we could use it if we want to update a cache file for some reason */
/* int geo_parser_get_geo_from_cache(Geo* _Geo, const char* _filepath)
{
  const char* geo_json_str = read_file_to_string(_Geo->cache_path);
  if (geo_json_str == NULL)
    return 0;

  cJSON* Json_Root = cJSON_Parse(geo_json_str);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"json error %s\n", error_pointer);
    }
    free((void*)geo_json_str);
    return 0;
  }

  int geo_count = json_get_int(Json_Root, "geo_count");
  
  cJSON* Json_Geo = cJSON_GetObjectItemCaseSensitive(Json_Root, "geo");
  if (Json_Geo == NULL) {
    fprintf(stderr, "'geo' section missing in cache json\n");
    cJSON_Delete(Json_Root);
    return -2;
  }

  printf("geo_json_str: \n%s\n", geo_json_str);
  printf("geo cache path: %s\n", _Geo->cache_path);
  printf("geo_count: %i\n", geo_count);
  printf("_Geo->count: %i\n", _Geo->count);

  for (int i = 0; i < geo_count; i++)
  {
    cJSON* Json_Geo = cJSON_GetArrayItem(Json_Geo, i);
    if (Json_Geo != NULL && i < _Geo->count)
    {
      _Geo->values[i]->lat            = json_get_double(Json_Geo, "latitude");
      _Geo->values[i]->lon            = json_get_double(Json_Geo, "longitude");

      _Geo->values[i]->country        = strdup(json_get_string(Json_Geo, "country")); 
      _Geo->values[i]->city           = strdup(json_get_string(Json_Geo, "city")); 
      _Geo->values[i]->county         = strdup(json_get_string(Json_Geo, "county")); 
      _Geo->values[i]->street         = strdup(json_get_string(Json_Geo, "street")); 
      _Geo->values[i]->postcode       = strdup(json_get_string(Json_Geo, "postcode")); 
      _Geo->values[i]->locality       = strdup(json_get_string(Json_Geo, "locality")); 
      _Geo->values[i]->timezone       = strdup(json_get_string(Json_Geo, "timezone")); 

      _Geo->values[i]->street_number  = json_get_int(Json_Geo, "street_number");

      memcpy(_Geo->values[i]->timezone_gmt, json_get_string(Json_Geo, "timezone_gmt"), 6);
      _Geo->values[i]->timezone_gmt[6] = '\0';
      memcpy(_Geo->values[i]->country_code, json_get_string(Json_Geo, "country_code"), 2);
      _Geo->values[i]->country_code[2] = '\0';

      printf("_Geo->values[i]->city: %s\n", _Geo->values[i]->city);
    }
  }

  cJSON_Delete(Json_Root);
  free((void*)geo_json_str);

  return geo_count;
} */

/** Builds json string and saves it to cache 
 * Last stage, everything in Geo should be populated by now */
char* geo_parser_build_json(Geo* _Geo)
{
  cJSON* Json_Root = cJSON_CreateObject();
  json_set_string(Json_Root, "query", _Geo->query);

  cJSON* Json_Geo = cJSON_CreateArray();

  for (int i = 0; i < _Geo->count; i++)
  {
    Geo_Values Geo_Values = *_Geo->values[i];
    if (Geo_Values.lat != 0 && Geo_Values.lon != 0) // don't add these to json
    {
      cJSON* Json_Geo = cJSON_CreateObject();

      json_set_double(Json_Geo, "latitude",  Geo_Values.lat);
      json_set_double(Json_Geo, "longitude", Geo_Values.lon);

      if(Geo_Values.country  != NULL && strcmp(Geo_Values.country, "Unknown") != 0)
        json_set_string(Json_Geo, "country", Geo_Values.country);

      if(Geo_Values.country_code[0]   != '\0') // is this viable?
        json_set_string(Json_Geo, "country_code", Geo_Values.country_code);

      if(Geo_Values.county   != NULL && strcmp(Geo_Values.county, "Unknown") != 0)
        json_set_string(Json_Geo, "county", Geo_Values.county);

      if(Geo_Values.city     != NULL && strcmp(Geo_Values.city, "Unknown") != 0)
        json_set_string(Json_Geo, "city", Geo_Values.city);

      if(Geo_Values.postcode != NULL && strcmp(Geo_Values.postcode, "Unknown") != 0)
        json_set_string(Json_Geo, "postcode", Geo_Values.postcode);

      if(Geo_Values.street   != NULL && strcmp(Geo_Values.street, "Unknown") != 0)
        json_set_string(Json_Geo, "street", Geo_Values.street);

      if (Geo_Values.house_number != NULL && strcmp(Geo_Values.house_number, "Unknown") != 0)
        json_set_string(Json_Geo, "house_number", Geo_Values.house_number);

      if (Geo_Values.locality != NULL && strcmp(Geo_Values.locality, "Unknown") != 0)
        json_set_string(Json_Geo, "locality", Geo_Values.locality);

      if(Geo_Values.timezone != NULL && strcmp(Geo_Values.timezone, "Unknown") != 0)
        json_set_string(Json_Geo, "timezone", Geo_Values.timezone);
      
      if(Geo_Values.timezone_gmt[0]   != '\0')
        json_set_string(Json_Geo, "timezone_gmt", Geo_Values.timezone_gmt);

      cJSON_AddItemToArray(Json_Geo, Json_Geo);
    }

  }
  cJSON_AddItemToObject(Json_Root, "geo", Json_Geo);
  char* json_str = cJSON_Print(Json_Root); // Uses realloc and ends up in heap

  if (write_string_to_file(json_str, _Geo->cache_path) != 0)
    fprintf(stderr, "FAILED TO WRITE STRING \"%p\" TO CACHE \"%s\"\n", json_str, _Geo->cache_path); 

  cJSON_Delete(Json_Root);

  return json_str;
}

/** Takes a string and tries to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful parse, 0 if none parsed and -1 on error */
int geo_parser_lat_lon(const char* _val, float* _target_coord)
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

void geo_parser_taskwork(void* _context, uint64_t _montime) {
  if (!_context) {
    return;
  }
  
  Geo_Parser* Parser = (Geo_Parser*)_context;

  switch(Parser->state)
  {
    case GEO_PARSER_INITIALIZING: {
      printf("GEO_PARSER_INITIALIZING\n");
      break;
    }
    case GEO_PARSER_CHECKING_CACHE: {
      printf("GEO_PARSER_CHECKING_CACHE\n");
      Parser->state = geo_parser_worktask_check_cache(Parser);
      break;
    }
    case GEO_PARSER_CALLING_EXT_API: {
      printf("GEO_PARSER_CALLING_EXT_API\n");
      Parser->state = geo_parser_worktask_call_ext_api(Parser);
      break;
    }
    case GEO_PARSER_IDLING: {
      break;
    }
    case GEO_PARSER_PARSING: {
      printf("GEO_PARSER_PARSING\n");
      Parser->state = geo_parser_worktask_parse_api_response(Parser);
      break;
    }
    case GEO_PARSER_RESPONDING: {
      printf("GEO_PARSER_RESPONDING\n");
      Parser->state = geo_parser_worktask_respond(Parser);
      break;
    }
    case GEO_PARSER_DISPOSING: {
      printf("GEO_PARSER_DISPOSING\n");
      geo_parser_dispose_ptr(&Parser);
      break;
    }
    case WEATHER_PARSER_ERROR: {
      printf("WEATHER_PARSER_ERROR\n");
      fprintf(stderr, "geo_parser_taskwork failed, Geo_Parser: %p\n", Parser);
      weather_parser_dispose_ptr(&Parser);
      break;
    }

    default:
      break;
  }
}


void geo_parser_dispose_ptr(Geo** _Gs_Ptr)
{
  /* Dispose of Geo */
  if (*_Gs_Ptr != NULL)
  {
    if ((*_Gs_Ptr)->query != NULL)
    {
      free((void*)(*_Gs_Ptr)->query);
      (*_Gs_Ptr)->query = NULL;
    }
    if ((*_Gs_Ptr)->cache_path != NULL)
    {
      free((void*)(*_Gs_Ptr)->cache_path);
      (*_Gs_Ptr)->cache_path = NULL;
    }
    
    for (int i = 0; i < (*_Gs_Ptr)->count; i++)
    {
      if ((*_Gs_Ptr)->values[i] != NULL)
      {
        if ((*_Gs_Ptr)->values[i]->locality != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->locality);
          (*_Gs_Ptr)->values[i]->locality = NULL;
        }
        if ((*_Gs_Ptr)->values[i]->city != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->city);
          (*_Gs_Ptr)->values[i]->city = NULL;
        }
        if ((*_Gs_Ptr)->values[i]->country != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->country);
          (*_Gs_Ptr)->values[i]->country = NULL;
        }
        if ((*_Gs_Ptr)->values[i]->timezone != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->timezone);
          (*_Gs_Ptr)->values[i]->timezone = NULL;
        }
        if ((*_Gs_Ptr)->values[i]->county != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->county);
          (*_Gs_Ptr)->values[i]->county = NULL;
        }
        if ((*_Gs_Ptr)->values[i]->postcode != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->postcode);
          (*_Gs_Ptr)->values[i]->postcode = NULL;
        }
        if ((*_Gs_Ptr)->values[i]->street != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->street);
          (*_Gs_Ptr)->values[i]->street = NULL;
        }
        if ((*_Gs_Ptr)->values[i]->house_number != NULL)
        {
          free((void*)(*_Gs_Ptr)->values[i]->house_number);
          (*_Gs_Ptr)->values[i]->house_number = NULL;
        }
        free((*_Gs_Ptr)->values[i]);
        (*_Gs_Ptr)->values[i] = NULL;
      }
    }
    free((*_Gs_Ptr)->values);
    free(*_Gs_Ptr);
    *_Gs_Ptr = NULL;
  }
  _Gs_Ptr = NULL;
}
