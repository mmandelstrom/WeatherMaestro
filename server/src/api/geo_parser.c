#include "api/geo_parser.h"
#include "cJSON.h"
#include "error.h"
#include "json_utils.h"
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
int geo_parser_init_geo_ptr(Geo** _GEO_Ptr, int _count); 

/** Builds and returns the parsed json output from Geo struct
 * Also saves cache file */
char* geo_parser_build_json(Geo* _Geo);


/* Helpers to allocate geo and count*/
static int geo_parser_ensure_capacity(Geo* _Geo, size_t _needed);
static int geo_parser_set_count(Geo* _Geo, size_t _new_count);
static void geo_values_clear_strings(Geo_Values* _Geo_vals);
/*-------------------------------------------------------------*/

/* TASKWORK */
void geo_parser_on_ext_api_finish(void* _context, void* _ext_api);

void geo_parser_taskwork(void* _context, uint64_t _montime);
GeoParserState geo_parser_worktask_check_cache(Geo_Parser* _Parser);
GeoParserState geo_parser_worktask_call_ext_api(Geo_Parser* _Parser);
GeoParserState geo_parser_worktask_parse_api_response(Geo_Parser* _Parser);
GeoParserState geo_parser_worktask_respond(Geo_Parser* _Parser);
void geo_dispose_ptr(Geo** _G_ptr);
/* ----------------------------------------------------------------- */


int geo_parser_init_ptr(Geo_Parser** _Parser_Ptr, void* _context, parser_on_finish _on_finish, bool _use_query, const char* _query, float _lat, float _lon)
{
  if (_Parser_Ptr == NULL) {
    return ERR_INVALID_ARG;
  } 

  *_Parser_Ptr = calloc(1, sizeof(Geo_Parser));
  if (*_Parser_Ptr == NULL) {
    perror("malloc");
    return ERR_NO_MEMORY;
  }

  Geo* geo = NULL;
  int result = geo_parser_init_geo_ptr(&geo, 1);
  if (result != SUCCESS) {
    return result;
  }

  (*_Parser_Ptr)->context = _context;
  (*_Parser_Ptr)->on_finish = _on_finish;
  (*_Parser_Ptr)->geo = geo;
  (*_Parser_Ptr)->latitude = _lat;
  (*_Parser_Ptr)->longitude = _lon;
  (*_Parser_Ptr)->use_query = _use_query;
  (*_Parser_Ptr)->task = scheduler_create_task((*_Parser_Ptr), geo_parser_taskwork);

  if (_use_query && _query) {
    (*_Parser_Ptr)->query = strdup(_query);
  }
  if ((*_Parser_Ptr)->query == NULL) {
    geo_parser_dispose_ptr(_Parser_Ptr);
    return ERR_NO_MEMORY;
  } 

  if (!(*_Parser_Ptr)->task) {
    geo_parser_dispose_ptr(_Parser_Ptr);
    return ERR_INTERNAL;
  }

  (*_Parser_Ptr)->state = GEO_PARSER_CHECKING_CACHE;

  return SUCCESS;
}

int geo_parser_init_geo_ptr(Geo** _GEO_Ptr, int _initial_capacity) {
  if (!_GEO_Ptr || !_initial_capacity) {
    return ERR_INVALID_ARG;
  }

  Geo* g = calloc(1, sizeof(Geo));
  if (!g) {
    perror("calloc");
    return ERR_NO_MEMORY;
  }

  g->values = calloc((size_t)_initial_capacity, sizeof(*g->values));
  if (!g->values) {
    perror("calloc");
    free(g);
    return ERR_NO_MEMORY;
  }

  g->capacity = (size_t)_initial_capacity;
  g->count = 0;

  *_GEO_Ptr = g;

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
   if (_Parser->json_output == NULL) {
      fprintf(stderr, "Failed to read cache file: %s\n", _Parser->geo->cache_path);
      return GEO_PARSER_CALLING_EXT_API;
    }

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
    _Parser->geo->query = _Parser->query; 
    
    int result = nominatim_get_geo(&_Parser->nom,
                                   true,
                                   _Parser->query,
                                   0.0f,
                                   0.0f,
                                   geo_parser_on_ext_api_finish,
                                   _Parser);

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

void geo_parser_on_ext_api_finish(void* _context, void* _ext_api) {
  if (_context == NULL) {
    return;
  }
  
  Geo_Parser* Parser = (Geo_Parser*)_context;

  Parser->nom_result = (Nominatim_Result*)_ext_api;

  if (Parser->nom_result == NULL) {
    Parser->state = GEO_PARSER_ERROR;
    return;
  }

  Parser->state = GEO_PARSER_PARSING;
}

GeoParserState geo_parser_worktask_parse_api_response(Geo_Parser* _Parser) {

  if (_Parser == NULL || _Parser->nom_result == NULL || _Parser->geo == NULL) {
    return GEO_PARSER_ERROR;
  }
  
  int result = geo_parser_parse_nominatim_geo(_Parser->geo,
                                              _Parser->nom_result->geo,
                                              _Parser->nom_result->count);

  if (result != SUCCESS) {
    return GEO_PARSER_ERROR;
  }

  //Geo is populated we can dispose nominatim
  nominatim_dispose(&_Parser->nom);
  _Parser->nom_result = NULL;

  return GEO_PARSER_RESPONDING;
}


static int geo_parser_set_count(Geo* _Geo, size_t _new_count);
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

GeoParserState geo_parser_worktask_respond(Geo_Parser* _Parser) {
  if (_Parser == NULL) {
    return GEO_PARSER_ERROR;
  }

  //If we already have json saved
  if (_Parser->json_output != NULL) {
    char* out = _Parser->json_output;
    _Parser->json_output = NULL;
    _Parser->on_finish(_Parser->context, &out);
    return GEO_PARSER_DISPOSING;
  }
  
  if (_Parser->geo == NULL) {
    return GEO_PARSER_ERROR;
  }

  char* response = geo_parser_build_json(_Parser->geo);
  if (response == NULL) {
    return GEO_PARSER_ERROR;
  }

  _Parser->on_finish(_Parser->context, &response);

  return GEO_PARSER_DISPOSING;
}

static int geo_parser_ensure_capacity(Geo* _Geo, size_t _needed) {
  if (!_Geo) {
    return ERR_INVALID_ARG;
  }
  
  if (_needed <= _Geo->capacity) {
    return SUCCESS;
  }
  
  size_t new_cap = _Geo->capacity ? _Geo->capacity : 1;

  while (new_cap < _needed) {
    new_cap *= 2;
  }

  Geo_Values* temp = realloc(_Geo->values, new_cap * sizeof(*temp));
  if (!temp) {
    return ERR_NO_MEMORY;
  }

  //Make sure newly allocated is nulled
  memset(temp + _Geo->capacity, 0, (new_cap - _Geo->capacity) * sizeof(*temp));
  
  _Geo->values = temp;
  _Geo->capacity = new_cap;

  return SUCCESS;
}

static int geo_parser_set_count(Geo* _Geo, size_t _new_count) {
  
  int result = geo_parser_ensure_capacity(_Geo, _new_count);

  if (result != SUCCESS) {
    return result;
  } 

  _Geo->count = _new_count;
  return SUCCESS;

}

static void geo_values_clear_strings(Geo_Values* _Geo_vals) {
  if (!_Geo_vals) {
    return;
  }

  free(_Geo_vals->country);      _Geo_vals->country = NULL;
  free(_Geo_vals->county);       _Geo_vals->county = NULL;
  free(_Geo_vals->city);         _Geo_vals->city = NULL;
  free(_Geo_vals->postcode);     _Geo_vals->postcode = NULL;
  free(_Geo_vals->street);       _Geo_vals->street = NULL;
  free(_Geo_vals->house_number); _Geo_vals->house_number = NULL;
  free(_Geo_vals->locality);     _Geo_vals->locality = NULL;
  free(_Geo_vals->timezone);     _Geo_vals->timezone = NULL;

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
//
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

     result = bigdatacloud_get_geo_by_coords(BDC_Geo, _Geo->values[0].lat, _Geo->values[0].lon);
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

int geo_parser_parse_nominatim_geo(Geo* _Geo, Nominatim_Geo* _NOM_Geo, int _geo_count)
{
  if (!_Geo || !_NOM_Geo || _geo_count <= 0) {
    return ERR_INVALID_ARG;
  }
  
  int result = geo_parser_set_count(_Geo, (size_t)_geo_count); 
  if (result != SUCCESS) {
    return result;
  }

  for (int i = 0; i < _geo_count; i++) {
    Geo_Values* val = &_Geo->values[i];
    geo_values_clear_strings(val);

    val->lat = (float)_NOM_Geo[i].lat;
    val->lon = (float)_NOM_Geo[i].lon;

    //TODO: Make this if nicer
    if (_NOM_Geo[i].country_code[0] && _NOM_Geo[i].country_code[1]) {
      memcpy(val->country_code, _NOM_Geo[i].country_code, 2);
      val->country_code[2] = '\0';
    } else {
      val->country_code[0] = '\0';
    }
    
    if (_NOM_Geo[i].country)      val->country = strdup(_NOM_Geo[i].country);
    if (_NOM_Geo[i].county)       val->county = strdup(_NOM_Geo[i].county);
    if (_NOM_Geo[i].city)         val->city = strdup(_NOM_Geo[i].city);
    if (_NOM_Geo[i].postcode)     val->postcode = strdup(_NOM_Geo[i].postcode);
    if (_NOM_Geo[i].road)         val->street = strdup(_NOM_Geo[i].road);
    if (_NOM_Geo[i].house_number) val->house_number = strdup(_NOM_Geo[i].house_number);

  } 
  return SUCCESS;
}

int geo_parser_parse_bigdatacloud_geo(Geo* _Geo, Bigdatacloud_Geo* _BDC_Geo)
{
  if (!_Geo || !_BDC_Geo) {
    return ERR_INVALID_ARG;
  }

  int result = geo_parser_set_count(_Geo, 1);
  if (result != SUCCESS) {
    return result;
  }

  Geo_Values* v = &_Geo->values[0];
  geo_values_clear_strings(v);

  v->lat = (float)_BDC_Geo->latitude;
  v->lon = (float)_BDC_Geo->longitude;

  if (_BDC_Geo->country_code[0] && _BDC_Geo->country_code[1]) {
    memcpy(v->country_code, _BDC_Geo->country_code, 2);
    v->country_code[2] = '\0';
  } else {
    v->country_code[0] = '\0';
  }
  
  v->city     = _BDC_Geo->city         ? strdup(_BDC_Geo->city)         : NULL;
  v->country  = _BDC_Geo->country_name ? strdup(_BDC_Geo->country_name) : NULL;
  v->locality = _BDC_Geo->locality     ? strdup(_BDC_Geo->locality)     : NULL;

  return SUCCESS;
}
 //
 // int geo_parser_parse_bigdatacloud_geo(Geo* _Geo, Bigdatacloud_Geo* _BDC_Geo)
 // {
 //   _Geo->values->lat = _BDC_Geo->latitude; // should these be updated here?
 //   _Geo->values->lon = _BDC_Geo->longitude;
 //
 //   /* Assign string members */
 //   memcpy(_Geo->country_code, _BDC_Geo->country_code, 2);
 //   _Geo->country_code[2] = '\0';
 //
 //   _Geo->city         = strdup(_BDC_Geo->city);
 //   _Geo->country      = strdup(_BDC_Geo->country_name);
 //   _Geo->locality     = strdup(_BDC_Geo->locality);
 //
 //   if (_Geo->locality == NULL ||
 //       _Geo->country  == NULL || 
 //       _Geo->city     == NULL)
 //   {
 //     perror("Failed to duplicate bigdatacloud strings");
 //     return -1;
 //   }
 //
 //   printf("_BDC_Geo->latitude: %f", _BDC_Geo->latitude);
 //   printf("_Geo->latitude: %f", _Geo->lat);
 //
 //   return 0;
 // }
 //
// /** Pre-reqs either _Geo->query set or _Geo->values[0] lat and lon set */
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
     if (!_Parser->query) {
      return NULL;
    }

     int query_len = strlen(_Parser->query);
     const char* hashed_filename = MD5_HashToString(_Parser->query, query_len);

     filepath_len = cache_dir_len + strlen(hashed_filename) + file_ext_len;
     char* full_filepath = malloc(filepath_len + 1);
     if (full_filepath == NULL)
     {
       perror("malloc");
       return NULL;
     }
     snprintf(full_filepath, filepath_len + 1, "%s%s%s",
         CACHE_DIR, hashed_filename, file_ext);

     return full_filepath;
   }
   else if (_Parser->use_query == false) // use lat+lon for filename via CACHE_FILENAME_LOCATION template
   {
     filename_len = snprintf(filename_buf, 256,
           CACHE_FILENAME_GEO_COORDS,
           _Parser->geo->values[0].lat,
           _Parser->geo->values[0].lon);
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
  if (!_Geo) {
    return NULL;
  }

  cJSON* Json_Root = cJSON_CreateObject();
  if (!Json_Root) {
    return NULL;
  }

  json_set_string(Json_Root, "query", _Geo->query);

  cJSON* Json_Geo = cJSON_CreateArray();
  if (!Json_Geo) {
    cJSON_Delete(Json_Root);
    return NULL;
  }

    for (int i = 0; i < _Geo->count; i++) {

      Geo_Values* Geo_Values = &_Geo->values[i];
      // Don't add these
      if (Geo_Values->lat == 0.0f && Geo_Values->lon == 0.0f) {
        continue;
      } 
      
      cJSON* Geo_obj = cJSON_CreateObject();
      if (!Geo_obj) {
        continue;
      }
      
      json_set_double(Geo_obj, "latitude", Geo_Values->lat);
      json_set_double(Geo_obj, "longitude", Geo_Values->lon);

      if (Geo_Values->country && strcmp(Geo_Values->country, "Unknown") != 0) {
        json_set_string(Geo_obj, "country", Geo_Values->country);
      }

      if (Geo_Values->country_code[0] != '\0') {
        json_set_string(Geo_obj, "country_code", Geo_Values->country_code);
      }

      if (Geo_Values->county && strcmp(Geo_Values->county, "Unknown") != 0) {
        json_set_string(Geo_obj, "county", Geo_Values->county);
      }

      if (Geo_Values->city && strcmp(Geo_Values->city, "Unknown") != 0) {
        json_set_string(Geo_obj, "city", Geo_Values->city);
      }

      if (Geo_Values->postcode && strcmp(Geo_Values->postcode, "Unknown") != 0) {
        json_set_string(Geo_obj, "postcode", Geo_Values->postcode);
      }

      if (Geo_Values->street && strcmp(Geo_Values->street, "Unknown") != 0) {
        json_set_string(Geo_obj, "street", Geo_Values->street);
      }

      if (Geo_Values->house_number && strcmp(Geo_Values->house_number, "Unknown") != 0) {
        json_set_string(Geo_obj, "house_number", Geo_Values->house_number);
      }

      if (Geo_Values->locality && strcmp(Geo_Values->locality, "Unknown") != 0) {
        json_set_string(Geo_obj, "locality", Geo_Values->locality);
      }

      if (Geo_Values->timezone && strcmp(Geo_Values->timezone, "Unknown") != 0) {
        json_set_string(Geo_obj, "timezone", Geo_Values->timezone);
      }

      if (Geo_Values->timezone_gmt[0] != '\0') {
        json_set_string(Geo_obj, "timezone_gmt", Geo_Values->timezone_gmt);
      }

      cJSON_AddItemToArray(Json_Geo, Geo_obj);
    }
    
  cJSON_AddItemToObject(Json_Root, "geo", Json_Geo);
  char* json_str = cJSON_Print(Json_Root);
  cJSON_Delete(Json_Root);

  if (json_str && _Geo->cache_path) {
    if (write_string_to_file(json_str, _Geo->cache_path) != 0) {
      fprintf(stderr, "FAILED TO WRITE STRING \"%p\" TO CACHE \"%s\"\n", json_str, _Geo->cache_path);
    }
  }
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
    case GEO_PARSER_ERROR: {
      printf("WEATHER_PARSER_ERROR\n");
      fprintf(stderr, "geo_parser_taskwork failed, Geo_Parser: %p\n", Parser);
      geo_parser_dispose_ptr(&Parser);
      break;
    }

    default:
      break;
  }
}

void geo_dispose_ptr(Geo** _Geo_Ptr)
{
  if (!_Geo_Ptr || !*_Geo_Ptr) {
    return;
  } 

  Geo* g = *_Geo_Ptr;

  g->query = NULL;

  free(g->cache_path);
  g->cache_path = NULL;

  if (g->values) {
    for (size_t i = 0; i < g->count; i++) {
      geo_values_clear_strings(&g->values[i]);
    }
    free(g->values);
    g->values = NULL;
  }

  g->count = 0;
  g->capacity = 0;

  free(g);
  *_Geo_Ptr = NULL;
}


void geo_parser_dispose_ptr(Geo_Parser** _GP_ptr)
{
  if (!_GP_ptr || !*_GP_ptr) return;
  Geo_Parser* P = *_GP_ptr;

  if (P->task) {
    scheduler_destroy_task(P->task);
    P->task = NULL;
  }

  if (P->nom) {
    nominatim_dispose(&P->nom);
    P->nom = NULL;
  }
  P->nom_result = NULL;

  if (P->json_output) {
    free(P->json_output);
    P->json_output = NULL;
  }

  if (P->query) {
    free((void*)P->query);
    P->query = NULL;
  }

  if (P->geo) {
    geo_dispose_ptr(&P->geo);
    P->geo = NULL;
  }

  P->context = NULL;
  P->on_finish = NULL;

  free(P);
  *_GP_ptr = NULL;
}

