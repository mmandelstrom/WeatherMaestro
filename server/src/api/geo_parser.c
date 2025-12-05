#include "../../include/api/geo_parser.h"

/* Pre-hashed cache filename definitions */
#define CACHE_FILENAME_GEO_COORDS "lat%.3f_lon%.3f_geo"
#define CACHE_FILENAME_GEO_QUERY "%s_geo"

/* ---------------------- Internal functions ----------------------- */

/** Heap init for single Geo struct. 
 * To init its weather/forecast struct, pass true as that arg */
int geo_parser_init_ptr(Geo** _G_Ptr, bool _weather, bool _forecast);

/** Returns the full filepath for a cache file
 * If _Geo->query is not NULL it will return a cache name by that query string, 
 * else look for lat and lon values 
 * Writes to heap, free'd by caller */
char* geo_parser_get_cache_filepath(Geos* _Geo);

/** */
int geo_parser_get_geo_from_cache(Geos* _Geos, const char* _filepath);

int geo_parser_parse_bigdatacloud_geo(Geos* _Geos, Bigdatacloud_Geo* _BDC_Geo);
int geo_parser_parse_nominatim_geo(Geos* _Geos, Nominatim_Geo* _NOM_Geo, int _geo_count);

int geo_parser_get_geo_from_api_by_coords(Geos* _Geos, ExternalGeoAPI _ExtAPI);
int geo_parser_get_geo_from_api_by_query(Geos* _Geos, ExternalGeoAPI ExtAPI);

/** Builds and returns the parsed json output from Geo struct
 * Also saves cache file 
 * Geo pointer(s) need to be wrapped in a Geos struct before calling */
char* geo_parser_build_json_geo(Geos* _Geos);

/* ----------------------------------------------------------------- */


int geo_parser_init_ptr(Geo** _G_Ptr, bool _weather, bool _forecast)
{
  if (_G_Ptr != NULL)
  {
    *_G_Ptr = malloc(sizeof(Geo));
    if (*_G_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_G_Ptr, 0, sizeof(Geo));

    /* Init Weather/Forecast structs */
    if (_weather && _forecast)
      weather_parser_init_ptr(&(*_G_Ptr)->weather, &(*_G_Ptr)->forecast);
    else if (_weather)
      weather_parser_init_ptr(&(*_G_Ptr)->weather, NULL);
    else if (_forecast)
      weather_parser_init_ptr(NULL, &(*_G_Ptr)->forecast);
  }

  return SUCCESS;
}

int geo_parser_init_geos_ptr(Geos** _Gs_Ptr, int _count, bool _weather, bool _forecast)
{

  if (_Gs_Ptr != NULL && _count > 0)
  {
    *_Gs_Ptr = malloc(sizeof(Geos));
    if (*_Gs_Ptr == NULL)
    {
      perror("malloc");
      return ERR_NO_MEMORY;
    }
    memset(*_Gs_Ptr, 0, sizeof(Geos));

    /* Allocate _count amount of pointers */
    (*_Gs_Ptr)->geo = malloc(sizeof(Geo*) * _count);

    int result;
    for (int i = 0; i < _count; i++)
    {
      result = geo_parser_init_ptr((*_Gs_Ptr)[i].geo, _weather, _forecast);
      if (result != SUCCESS)
      {
        perror("geo_parser_init_ptr");
        

      }


    }
  }
  return -1; // none initialized
}

/** Builds Geo structs using cache or external API by query
 * If the same query has been made before then there is a cache */
int geo_parser_get_geo_by_query(Geos* _Geos, const char* _query, char* _json_output)
{
  /*TODO: coordinate error codes so weather_api can decide whether the error was because of an internal server error or because the client request was faulty */

  int result;
  _Geos->geo[0]->query = _query;
  printf("_Geos->geo[0]->query: %s\n", _Geos->geo[0]->query);

  _Geos->geo[0]->cache_path = geo_parser_get_cache_filepath(_Geos->geo[0]);

  if (file_exists(_Geo->cache_path))
  {
    printf("Getting geo from cache\n");
    result = geo_parser_get_geo_from_cache(_Geo, _Geo->cache_path);
    if (result != 0)
    {
      perror("geo_parser_get_geo_from_cache");
      return -2;
    }
  }
  else
  {
    result = geo_parser_get_geo_from_api_by_query(_Geo, NOMINATIM_OSM_GEO);
    if (result != 0)
    {
      perror("geo_parser_get_geo_from_api_by_query");
      return -2;
    }

    _json_output = geo_parser_build_json_geo(_Geo);

  }
  return 0;
}

/** NOT IMPLEMENTED First looks in cache by lat and lon, if none found search via API */
int weather_parser_get_geo_by_coords(Geo* _Geo, char* _json_output)
{
  /*TODO: coordinate error codes so weather_api can decide whether the error was because of an internal server error or because the client request was faulty */

  ExternalGeoAPI ExtAPI = BIGDATACLOUD_GEO;
  int result;

  _Geo->cache_path = geo_parser_get_cache_filepath(_Geo);

  if (file_exists(_Geo->cache_path))
  {
    printf("Getting geo from cache\n");
    result = geo_parser_get_geo_from_cache(_Geo, _Geo->cache_path);
    if (result != 0)
    {
      perror("geo_parser_get_geo_from_cache");
      return -1;
    }
  }
  else // Get from api
  {
    printf("Getting geo from API\n");
    result = geo_parser_get_geo_from_api_by_coords(_Geo, BIGDATACLOUD_GEO);
    if (result != 0)
    {
      perror("geo_parser_get_geo_from_api_by_coords");
      return -2;
    }
    
  }

    _json_output = geo_parser_build_json_geo(_Geo);
  return 0;
}

int geo_parser_get_geo_from_api_by_query(Geo* _Geo, ExternalGeoAPI _ExtAPI)
{
  int result;

  if (!_Geo)
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
    int geo_count;
    result = nominatim_get_geo_by_query(NOM_Geo, geo_count, _Geo->query);
    if (result != 0)
    {
      perror("nominatim_get_weather");
      nominatim_dispose_ptr(&NOM_Geo);
      return -3;
    }

    result = geo_parser_parse_nominatim_geo(_Geo, NOM_Geo, geo_count);
    if (result != 0)
    {
      perror("weather_parser_parse_nominatim_weather");
      nominatim_dispose_ptr(&NOM_Geo);
      return -4;
    }
    nominatim_dispose_ptr(&NOM_Geo);

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
    int _geo_count = 0;
    /* result = nominatim_get_geo_by_coords(NOM_Geo, _Geo->lat, _Geo->lon); */
    if (result != 0)
    {
      perror("nominatim_get_geo_by_coords");
      nominatim_dispose_ptr(&NOM_Geo);
      return -3;
    }
    
    printf("---Nominatim Geo---\ncityname: %s | country %s\n lat: %f, lon: %f\n\n", NOM_Geo->city, NOM_Geo->country, NOM_Geo->lat, NOM_Geo->lon);

    result = geo_parser_parse_nominatim_geo(_Geo, NOM_Geo, _geo_count);
    if (result != 0)
    {
      perror("geo_parser_parse_nominatim_geo");
      nominatim_dispose_ptr(&NOM_Geo);
      return -4;
    }
    nominatim_dispose_ptr(&NOM_Geo);

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

    result = bigdatacloud_get_geo_by_coords(BDC_Geo, _Geo->lat, _Geo->lon);
    if (result != 0)
    {
      perror("bigdatacloud_get_geo_by_coords");
      bigdatacloud_dispose_ptr(&BDC_Geo);
      return -3;
    }

    printf("---Bigdatacloud Geo---\ncityname: %s | country %s\n lat: %f, lon: %f\n\n", BDC_Geo->city, BDC_Geo->country_name, BDC_Geo->latitude, BDC_Geo->longitude);

    result = geo_parser_parse_bigdatacloud_geo(_Geo, BDC_Geo);
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

int weather_parser_parse_nominatim_geo(Geo* _Geo, Nominatim_Geo* _NOM_Geo, int _geo_count)
{
  /* Reallocate more memory for Geo if there are more than one Nom_Geo structs */
  if (_geo_count > 1)
  {
    _Geo = realloc(_Geo, _geo_count * sizeof(Geo));
    if (_Geo == NULL)
    {
      perror("realloc");
      return -1;
    }
  }

  for (int i = 0; i < _geo_count; i++)
  {
    _Geo[i].lat = _NOM_Geo[i].lat;
    _Geo[i].lon = _NOM_Geo[i].lon;

    /* Assign string members */
    memcpy(_Geo[i].country_code, _NOM_Geo[i].country_code, 2);
    _Geo[i].country_code[2] = '\0';

    if(_NOM_Geo[i].country  != NULL)
      _Geo[i].country  = strdup(_NOM_Geo[i].country );
    if(_NOM_Geo[i].county   != NULL)
      _Geo[i].county   = strdup(_NOM_Geo[i].county  );
    if(_NOM_Geo[i].city     != NULL)
      _Geo[i].city     = strdup(_NOM_Geo[i].city    );
    if(_NOM_Geo[i].postcode != NULL)
      _Geo[i].postcode = strdup(_NOM_Geo[i].postcode);
    if(_NOM_Geo[i].street   != NULL)
      _Geo[i].street   = strdup(_NOM_Geo[i].street  );

    if (_NOM_Geo[i].house_number > 0)
      _Geo[i].street_number = _NOM_Geo[i].house_number;

    printf("_NOM_Geo[i].latitude: %f", _NOM_Geo[i].lat);
    printf("_Geo[i].latitude: %f", _Geo[i].lat);
  }

  return 0;
}

int weather_parser_parse_bigdatacloud_geo(Geo* _Geo, Bigdatacloud_Geo* _BDC_Geo)
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

int geo_parser_get_geo_from_cache(Geos* _Geos, const char* _filepath)
{
  const char* geo_json = read_file_to_string(_filepath);
  if (geo_json == NULL)
    return 0;

  cJSON* Json_Root = cJSON_Parse(geo_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"meteo json error %s\n", error_pointer);
    }
    free((void*)geo_json);
    return 0;
  }
  free((void*)geo_json);

  int geo_count         = json_get_int(Json_Root, "geo_count");

  for (int i = 0; i < geo_count; i++)
  {

    _Geo->lat            = json_get_double(Json_Root, "latitude");
    _Geo->lon            = json_get_double(Json_Root, "longitude");

    _Geo->country        = strdup(json_get_string(Json_Root, "country")); 
    _Geo->city           = strdup(json_get_string(Json_Root, "city")); 
    _Geo->county         = strdup(json_get_string(Json_Root, "county")); 
    _Geo->street         = strdup(json_get_string(Json_Root, "street")); 
    _Geo->postcode       = strdup(json_get_string(Json_Root, "postcode")); 
    _Geo->locality       = strdup(json_get_string(Json_Root, "locality")); 
    _Geo->timezone       = strdup(json_get_string(Json_Root, "timezone")); 

    _Geo->street_number  = json_get_int(Json_Root, "street_number");

    memcpy(_Geo->timezone_gmt, json_get_string(Json_Root, "timezone_gmt"), 6);
    _Geo->timezone_gmt[6] = '\0';
    memcpy(_Geo->country_code, json_get_string(Json_Root, "country_code"), 2);
    _Geo->country_code[2] = '\0';
  }

  cJSON_Delete(Json_Root);

  return geo_count;
}

/** Pre-reqs either */
char* geo_parser_get_cache_filepath(Geo* _Geo)
{
  char* file_ext = ".json";
  char filename_buf[256];

  int filename_len; 
  int file_ext_len = strlen(file_ext);
  int cache_dir_len = strlen(CACHE_DIR);
  int filepath_len; // full path

  if (_Geo->query != NULL) // use query for filename
  {
    int query_len = strlen(_Geo->query);
    const char* hashed_filename = MD5_HashToString(_Geo->query, query_len);

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
  else // use lat+lon for filename via CACHE_FILENAME_LOCATION template
  {
    filename_len = snprintf(filename_buf, 256,
          CACHE_FILENAME_LOCATION_QUERY,
          _Geo->lat,
          _Geo->lon);
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

char* weather_parser_build_json_geos(Geos* _Geos)
{
  cJSON* Json_Root = cJSON_CreateArray();

  for (int i = 0; i < _Geos->count; i++)
  {
    cJSON* Json_Geo = cJSON_CreateObject();
    Geo Geo = _Geos->geo[i];

    json_set_double(Json_Geo, "latitude",  Geo.lat);
    json_set_double(Json_Geo, "longitude", Geo.lon);

    if(Geo.country  != NULL && strcmp(Geo.country, "Unknown") != 0)
      json_set_string(Json_Geo, "country", Geo.country);

    if(Geo.country_code[0]   != '\0') // is this viable?
      json_set_string(Json_Geo, "country_code", Geo.country_code);

    if(Geo.county   != NULL && strcmp(Geo.county, "Unknown") != 0)
      json_set_string(Json_Geo, "county", Geo.county);

    if(Geo.city     != NULL && strcmp(Geo.city, "Unknown") != 0)
      json_set_string(Json_Geo, "city", Geo.city);

    if(Geo.postcode != NULL && strcmp(Geo.postcode, "Unknown") != 0)
      json_set_string(Json_Geo, "postcode", Geo.postcode);

    if(Geo.street   != NULL && strcmp(Geo.street, "Unknown") != 0)
      json_set_string(Json_Geo, "street", Geo.street);

    if (Geo.street_number > 0)
      json_set_int(Json_Geo, "street_number", Geo.street_number);

    if (Geo.locality != NULL && strcmp(Geo.locality, "Unknown") != 0)
      json_set_string(Json_Geo, "locality", Geo.locality);

    if(Geo.timezone != NULL && strcmp(Geo.timezone, "Unknown") != 0)
      json_set_string(Json_Geo, "timezone", Geo.timezone);
    
    if(Geo.timezone_gmt[0]   != '\0')
      json_set_string(Json_Geo, "timezone_gmt", Geo.timezone_gmt);

    cJSON_AddItemToArray(Json_Root, Json_Geo);

  }
  char* json_str = cJSON_Print(Json_Root); // Uses realloc and ends up in heap

  if (write_string_to_file(json_str, _Geos[0].geo->cache_path) != 0)
    fprintf(stderr, "FAILED TO WRITE STRING \"%p\" TO CACHE \"%s\"\n", json_str, _Geos[0].geo->cache_path); 

  cJSON_Delete(Json_Root);

  return json_str;
}

char* geo_parser_build_json(Geos* _Geos)
{
  cJSON* Json_Root = cJSON_CreateObject();

  json_set_int(Json_Root, "geo_count", _Geos->count);

  cJSON* Json_Geos = cJSON_CreateArray();

  for (int i = 0; i < 0; i++)
  {
    

    json_set_double(Json_Root, "latitude", _Geo->lat);
    json_set_double(Json_Root, "longitude", _Geo->lon);

    if(_Geo->country  != NULL && strcmp(_Geo->country, "Unknown") != 0)
      json_set_string(Json_Root, "country", _Geo->country);

    if(_Geo->country_code[0]   != '\0')
      json_set_string(Json_Root, "country_code", _Geo->country_code);

    if(_Geo->county   != NULL && strcmp(_Geo->county, "Unknown") != 0)
      json_set_string(Json_Root, "county", _Geo->county);
    
    if(_Geo->city     != NULL && strcmp(_Geo->city, "Unknown") != 0)
      json_set_string(Json_Root, "city", _Geo->city);

    if(_Geo->postcode != NULL && strcmp(_Geo->postcode, "Unknown") != 0)
      json_set_string(Json_Root, "postcode", _Geo->postcode);

    if(_Geo->street   != NULL && strcmp(_Geo->street, "Unknown") != 0)
      json_set_string(Json_Root, "street", _Geo->street);

    if (_Geo->street_number > 0)
      json_set_int(Json_Root, "street_number", _Geo->street_number);

    if (_Geo->locality != NULL && strcmp(_Geo->locality, "Unknown") != 0)
      json_set_string(Json_Root, "locality", _Geo->locality);

    if(_Geo->timezone != NULL && strcmp(_Geo->timezone, "Unknown") != 0)
      json_set_string(Json_Root, "timezone", _Geo->timezone);

    if(_Geo->timezone_gmt[0]   != '\0')
      json_set_string(Json_Root, "timezone_gmt", _Geo->timezone_gmt);
  }
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


void geo_parser_dispose_ptr(Geo** _L_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr)
{
  /* Dispose of Geo */
  if (_L_Ptr != NULL)
  {
    if (*_L_Ptr != NULL)
    {
      if ((*_L_Ptr)->forecast != NULL)
        weather_parser_dispose_ptr(NULL, &(*_L_Ptr)->forecast);
      if ((*_L_Ptr)->weather != NULL)
        weather_parser_dispose_ptr(&(*_L_Ptr)->weather, NULL);

      if ((*_L_Ptr)->query != NULL)
      {
        free((void*)(*_L_Ptr)->query);
        (*_L_Ptr)->query = NULL;
      }
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
      if ((*_L_Ptr)->county != NULL)
      {
        free((void*)(*_L_Ptr)->county);
        (*_L_Ptr)->county = NULL;
      }
      if ((*_L_Ptr)->postcode != NULL)
      {
        free((void*)(*_L_Ptr)->postcode);
        (*_L_Ptr)->postcode = NULL;
      }
      if ((*_L_Ptr)->street != NULL)
      {
        free((void*)(*_L_Ptr)->street);
        (*_L_Ptr)->street = NULL;
      }
      free(*_L_Ptr);
      *_L_Ptr = NULL;
    }
    _L_Ptr = NULL;
  }

}
