#include "api/nominatim.h"
#include "error.h"
#include "http_client.h"
#include <stdio.h>


/* ---------------------- Internal functions ----------------------- */

const char* nominatim_get_geocode_json_by_query(const char* _query);
int nominatim_parse_geocode_json(Nominatim_Geo** _NOM_Geo_Ptr, int* _geo_count, const char* _json);
int nominatim_get_geo_json(Nominatim* _NOM);
int nominatim_build_url(char* _out, size_t _out_size, Nominatim* _NOM);
void nominatim_on_http_client_finish(void* _context, char** _response);

/* ----------------------------------------------------------------- */
int nominatim_get_geo(Nominatim** _NOM, bool _use_query, const char* _query, float _lat, float _lon, on_ext_api_finish _on_finish, void* _context) {
  if (_NOM == NULL || _on_finish == NULL) {
    return ERR_INVALID_ARG;
  }

  Nominatim* N = calloc(1, sizeof(Nominatim));
  if (N == NULL) {
    perror("calloc");
    return ERR_NO_MEMORY;
  }

  N->result = calloc(1, sizeof(Nominatim_Result));
  if (!N->result) {
    perror("calloc");
    free(N);
    return ERR_NO_MEMORY;
  }

  N->on_finish = _on_finish;
  N->context = _context;
  N->use_query = _use_query;
  N->lat = _lat;
  N->lon = _lon;

  if (_use_query) {
    if (!_query) {
      free(N);
      return ERR_INVALID_ARG;
    }
    N->query = strdup(_query);
    if (!N->query) {
      free(N);
      return ERR_NO_MEMORY;
    }
  }

  N->result->geo = NULL;
  N->result->count = 0;


  *_NOM = N;

  int res = nominatim_get_geo_json(N);
  if (res != SUCCESS) {
    nominatim_dispose(&N);
    return res;
    }

  return SUCCESS;
  
}





int nominatim_init_ptr(Nominatim_Geo** _NOM_Geo_Ptr)
{

  if (_NOM_Geo_Ptr == NULL)
    return -1;

  *_NOM_Geo_Ptr = malloc(sizeof(Nominatim_Geo));
  if (*_NOM_Geo_Ptr == NULL)
  {
    perror("malloc");
    return -2;
  }

  memset(*_NOM_Geo_Ptr, 0, sizeof(Nominatim_Geo));

  return 0;
}

int nominatim_get_geo_by_query(Nominatim_Geo** _NOM_Geo, int* _geo_count, const char* _query)
{
  /* GET nominatim city json */
  const char* nominatim_json = nominatim_get_geocode_json_by_query(_query);
  if (nominatim_json == NULL)
  {
    perror("nominatim_get_weather_json");
    return -1;
  }

  /* Parse nominatim json to Nominatim_Geo struct */
  int result = nominatim_parse_geocode_json(_NOM_Geo, _geo_count, nominatim_json);
  if (result != 0)
  {
    perror("nominatim_parse_json");
    free((void*)nominatim_json);
    return result;
  }
  free((void*)nominatim_json);

  return 0;
}

int nominatim_build_url(char* _out, size_t _out_size, Nominatim* _NOM) {
  if (_out == NULL || _out_size == 0 || _NOM == NULL) {
    return ERR_INVALID_ARG;
  }

  int n = 0;
  
  if (_NOM->use_query == true) {
    if (_NOM->query == NULL) {
      return ERR_INVALID_ARG;
    }

    n = snprintf(_out, _out_size, NOMINATIM_GEOCODE_QUERY, _NOM->query);
  }

  if (n < 0 || (size_t)n >= _out_size) {
    return ERR_INTERNAL;
  }

  _out[n] = '\0';

  return SUCCESS;
}

int nominatim_get_geo_json(Nominatim* _NOM) {
  if (_NOM == NULL) {
    return ERR_INVALID_ARG;
  }

  char url[512];
  int res = nominatim_build_url(url, sizeof(url), _NOM);
  if (res != SUCCESS) {
    return res;
  }

  HTTP_Client* client = calloc(1, sizeof(HTTP_Client));
  if (client == NULL) {
    perror("malloc");
    return ERR_NO_MEMORY;
  }

  _NOM->http_client = client;
  HTTPMethod method = HTTP_GET;

  res = http_client_initiate(client,
                             url,
                             method,
                             nominatim_on_http_client_finish,
                             _NOM,
                             &_NOM->http_response);

  if (res != SUCCESS) {
    free(client);
    _NOM->http_client = NULL;
    return res;
  }
  
  return SUCCESS;

}

void nominatim_on_http_client_finish(void* _context, char** _response) {
  if (_context == NULL || _response == NULL || *_response == NULL) {
    return;
  }

  Nominatim *NOM = (Nominatim*)_context;

  int res = nominatim_parse_geocode_json(&NOM->result->geo,
                                         &NOM->result->count,
                                         *_response);

  free(*_response);
  *_response = NULL;
  NOM->http_response = NULL;

  if (res != SUCCESS) {
    NOM->on_finish(NOM->context, NULL);
  }

  NOM->on_finish(NOM->context, &NOM->result);
}

int nominatim_get_geo_by_coords(Nominatim_Geo* _NOM_Geo, float _lat, float _lon)
{

  return 0;
}

const char* nominatim_get_geocode_json_by_query(const char* _query)
{
  char url[512];

  int url_len = snprintf(url, 512, 
                         NOMINATIM_GEOCODE_QUERY,
                         _query);
  url[url_len] = '\0';
  
	Curl_Data C_Data;
	if (curl_init(&C_Data) != 0)
		return NULL;

  printf("\n--- CALLING NOMINATIM RESPONSE --- \n\n");
    
	int result = curl_get_response(&C_Data, url);
	if (result != 0)
	{
		perror("curl_get_response");
		curl_dispose(&C_Data);
		return NULL;
	}

  char* response = malloc(C_Data.size + 1);
  if (response == NULL)
  {
    perror("malloc");
		curl_dispose(&C_Data);
		return NULL;
  }

  memcpy(response, C_Data.addr, C_Data.size);
  response[C_Data.size] = '\0';
  curl_dispose(&C_Data);

  printf("===== Nominatim Response JSON =====\n\n%s\n\n", response);

  return response;
}

int nominatim_parse_geocode_json(Nominatim_Geo** _NOM_Geo_Ptr, int* _geo_count, const char* _json)
{
  cJSON* Json_Root = cJSON_Parse(_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"nominatim json error %s\n", error_pointer);
    }
    return -1;
  }

  (*_geo_count) = cJSON_GetArraySize(Json_Root);
  size_t array_size = (size_t)*_geo_count * sizeof(Nominatim_Geo);

  printf("Found %i geos from Nominatim API\n", *_geo_count);

  (*_NOM_Geo_Ptr) = realloc((*_NOM_Geo_Ptr), array_size);
  if ((*_NOM_Geo_Ptr) == NULL)
  {
    fprintf(stderr, "Failed to realloc memory for Nominatim_Geo array\n");
    cJSON_Delete(Json_Root);
    return ERR_NO_MEMORY;
  }
  memset((*_NOM_Geo_Ptr), 0, array_size);

  for (int i = 0; i < *_geo_count; i++)
  {

    cJSON* Json_Place = cJSON_GetArrayItem(Json_Root, i);
    if (Json_Place == NULL){
      fprintf(stderr, "no places found from nominatim\n");
      cJSON_Delete(Json_Root);
      return ERR_INVALID_ARG;
    }
    cJSON* Json_Address = cJSON_GetObjectItemCaseSensitive(Json_Place, "address");
    if (Json_Place == NULL){
      fprintf(stderr, "'current' section missing in meteo json\n");
      cJSON_Delete(Json_Root);
      return -3;
    }

    memcpy((*_NOM_Geo_Ptr)[i].country_code, json_get_string(Json_Address, "country_code"), 2);
    (*_NOM_Geo_Ptr)[i].country_code[2] = '\0';

    /* Parse lat and lon strings */
    char* lat = strdup(json_get_string(Json_Place, "lat"));
    char* lon = strdup(json_get_string(Json_Place, "lon"));

    if (lat == NULL || lon == NULL)
    {
      perror("strdup");
      cJSON_Delete(Json_Root);
      return -4;
    }

    if (parse_string_to_double(lat, &(*_NOM_Geo_Ptr)[i].lat) != 0 || parse_string_to_double(lon, &(*_NOM_Geo_Ptr)[i].lon) != 0)
    {
      perror("parse_string_to_double");
      free(lat); free(lon);
      cJSON_Delete(Json_Root);
      return -5;
    }
    free(lat); free(lon);

    printf("NOM_Geo[%i] lat: %lf lon: %lf\n", i, (*_NOM_Geo_Ptr)[i].lat, (*_NOM_Geo_Ptr)[i].lon);

    /* Heap allocations */
    (*_NOM_Geo_Ptr)[i].country       = strdup(json_get_string(Json_Address, "country"));
    (*_NOM_Geo_Ptr)[i].county        = strdup(json_get_string(Json_Address, "county"));
    (*_NOM_Geo_Ptr)[i].city          = strdup(json_get_string(Json_Address, "city"));
    (*_NOM_Geo_Ptr)[i].postcode      = strdup(json_get_string(Json_Address, "postcode"));
    (*_NOM_Geo_Ptr)[i].road          = strdup(json_get_string(Json_Address, "road"));
    (*_NOM_Geo_Ptr)[i].house_number  = strdup(json_get_string(Json_Address, "house_number"));

    if ((*_NOM_Geo_Ptr)[i].country      == NULL ||
        (*_NOM_Geo_Ptr)[i].county       == NULL ||
        (*_NOM_Geo_Ptr)[i].city         == NULL ||
        (*_NOM_Geo_Ptr)[i].postcode     == NULL ||
        (*_NOM_Geo_Ptr)[i].road         == NULL ||
        (*_NOM_Geo_Ptr)[i].house_number == NULL)
    {
      fprintf(stderr, "One or more strings couldn't be parsed from nominatim json\n");
      cJSON_Delete(Json_Root);
      return -6;
    }

  }

  cJSON_Delete(Json_Root);

  return 0;
}

void nominatim_dispose_ptr(Nominatim_Geo** _NOM_Geo_Ptr, int _count)
{

  if (_NOM_Geo_Ptr == NULL)
    return;

  if (*_NOM_Geo_Ptr == NULL)
  {
    _NOM_Geo_Ptr = NULL;
    return;
  }

  for (int i = 0; i < _count; i++)
  {
    if ((*_NOM_Geo_Ptr)[i].country != NULL)
    {
      free((void*)(*_NOM_Geo_Ptr)[i].country);
      (*_NOM_Geo_Ptr)[i].country = NULL;
    }
    if ((*_NOM_Geo_Ptr)[i].city != NULL)
    {
      free((void*)(*_NOM_Geo_Ptr)[i].city);
      (*_NOM_Geo_Ptr)[i].city = NULL;
    }
    if ((*_NOM_Geo_Ptr)[i].county != NULL)
    {
      free((void*)(*_NOM_Geo_Ptr)[i].county);
      (*_NOM_Geo_Ptr)[i].county = NULL;
    }
    if ((*_NOM_Geo_Ptr)[i].postcode != NULL)
    {
      free((void*)(*_NOM_Geo_Ptr)[i].postcode);
      (*_NOM_Geo_Ptr)[i].postcode = NULL;
    }
    if ((*_NOM_Geo_Ptr)[i].road != NULL)
    {
      free((void*)(*_NOM_Geo_Ptr)[i].road);
      (*_NOM_Geo_Ptr)[i].road = NULL;
    }
    if ((*_NOM_Geo_Ptr)[i].house_number != NULL)
    {
      free((void*)(*_NOM_Geo_Ptr)[i].house_number);
      (*_NOM_Geo_Ptr)[i].house_number = NULL;
    }
  }

  free(*_NOM_Geo_Ptr);

  *_NOM_Geo_Ptr = NULL;
  _NOM_Geo_Ptr = NULL;

}

void nominatim_dispose(Nominatim** _N_Ptr)
{
  if (!_N_Ptr || !*_N_Ptr) return;
  Nominatim* N = *_N_Ptr;

  if (N->query) {
    free(N->query);
    N->query = NULL;
  }

  // om result.geo har skapats av parsern:
  if (N->result->geo) {
    nominatim_dispose_ptr(&N->result->geo, N->result->count);
    N->result->count = 0;
  }


  free(N);
  *_N_Ptr = NULL;
}


/*
Example response with query "Arenavagen%2061%20Stockholm%20Sweden":

[
  {
    "place_id": 153940147,
    "licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright",
    "osm_type": "node",
    "osm_id": 1298718355,
    "lat": "59.2927684",
    "lon": "18.0812270",
    "class": "place",
    "type": "house",
    "place_rank": 30,
    "importance": 0.0000840184634164495,
    "addresstype": "place",
    "name": "",
    "display_name": "61, Arenavägen, Johanneshov, Enskede-Årsta-Vantörs stadsdelsområde, Stockholm, Stockholms kommun, Stockholms län, 121 62, Sverige",
    "address": {
      "house_number": "61",
      "road": "Arenavägen",
      "suburb": "Johanneshov",
      "city_district": "Enskede-Årsta-Vantörs stadsdelsområde",
      "city": "Stockholm",
      "municipality": "Stockholms kommun",
      "county": "Stockholms län",
      "ISO3166-2-lvl4": "SE-AB",
      "postcode": "121 62",
      "country": "Sverige",
      "country_code": "se"
    },
    "boundingbox": [
      "59.2927184",
      "59.2928184",
      "18.0811770",
      "18.0812770"
    ]
  }
]


Example response with query "Uppsala":


[
  {
    "place_id": 154042173,
    "licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright",
    "osm_type": "node",
    "osm_id": 25735371,
    "lat": "59.8586126",
    "lon": "17.6387436",
    "class": "place",
    "type": "city",
    "place_rank": 16,
    "importance": 0.6656801144720973,
    "addresstype": "city",
    "name": "Uppsala",
    "display_name": "Uppsala, Uppsala kommun, Uppsala län, 753 20, Sverige",
    "address": {
      "city": "Uppsala",
      "municipality": "Uppsala kommun",
      "county": "Uppsala län",
      "ISO3166-2-lvl4": "SE-C",
      "postcode": "753 20",
      "country": "Sverige",
      "country_code": "se"
    },
    "boundingbox": [
      "59.6986126",
      "60.0186126",
      "17.4787436",
      "17.7987436"
    ]
  }
]

*/
