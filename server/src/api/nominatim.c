#include "../../include/api/nominatim.h"


/* ---------------------- Internal functions ----------------------- */

const char* nominatim_get_geocode_json(const char* _query);
int nominatim_parse_geocode_json(Nominatim_Geo* _NOM_Geo, const char* _json);

/* ----------------------------------------------------------------- */

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

int nominatim_get_geo_by_query(Nominatim_Geo* _NOM_Geo, const char* _query)
{
  /* GET nominatim city json */
  const char* nominatim_json = nominatim_get_geocode_json(_query);
  if (nominatim_json == NULL)
  {
    perror("nominatim_get_weather_json");
    return -1;
  }

  /* Parse nominatim json to Nominatim_Geo struct */
  int result = nominatim_parse_geocode_json(_NOM_Geo, nominatim_json);
  if (result != 0)
  {
    perror("nominatim_parse_json");
    free((void*)nominatim_json);
    return result;
  }
  free((void*)nominatim_json);

  return 0;
}

const char* nominatim_get_geocode_json(const char* _query)
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

int nominatim_parse_geocode_json(Nominatim_Geo* _NOM_Geo, const char* _json)
{
  cJSON* Json_Root = cJSON_Parse(_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"nominatim json error %s\n", error_pointer);
    }
    return -1;
  }
  cJSON* Json_Place = cJSON_GetArrayItem(Json_Root, 0);
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

  memcpy(_NOM_Geo->country_code, json_get_string(Json_Address, "country_code"), 2);
  _NOM_Geo->country_code[2] = '\0';

  char* lat = strdup(json_get_string(Json_Place, "lat"));
  char* lon = strdup(json_get_string(Json_Place, "lon"));

  if (lat == NULL || lon == NULL)
  {
    perror("strdup");
    cJSON_Delete(Json_Root);
    return -4;
  }

  if (parse_string_to_double(lat, &_NOM_Geo->lat) != 0 || parse_string_to_double(lon, &_NOM_Geo->lon) != 0)
  {
    perror("parse_string_to_double");
    free(lat); free(lon);
    cJSON_Delete(Json_Root);
    return -5;
  }
  free(lat); free(lon);

  /* Heap allocations */
  _NOM_Geo->country   = strdup(json_get_string(Json_Address, "country"));
  _NOM_Geo->county    = strdup(json_get_string(Json_Address, "county"));
  _NOM_Geo->city      = strdup(json_get_string(Json_Address, "city"));
  _NOM_Geo->postcode  = strdup(json_get_string(Json_Address, "postcode"));
  _NOM_Geo->street    = strdup(json_get_string(Json_Address, "street"));

  if (_NOM_Geo->country   == NULL ||
      _NOM_Geo->county    == NULL ||
      _NOM_Geo->city      == NULL ||
      _NOM_Geo->postcode  == NULL ||
      _NOM_Geo->street    == NULL)
  {
    fprintf(stderr, "One or more strings couldn't be parsed from nominatim json\n");
    cJSON_Delete(Json_Root);
    return -4;
  }

  _NOM_Geo->house_number   = json_get_int(Json_Address, "house_number");

  cJSON_Delete(Json_Root);

  return 0;
}

void nominatim_dispose_ptr(Nominatim_Geo** _NOM_Geo_Ptr)
{

  if (_NOM_Geo_Ptr == NULL)
    return;

  if (*_NOM_Geo_Ptr == NULL)
  {
    _NOM_Geo_Ptr = NULL;
    return;
  }

  if ((*_NOM_Geo_Ptr)->country != NULL)
  {
    free((void*)(*_NOM_Geo_Ptr)->country);
    (*_NOM_Geo_Ptr)->country = NULL;
  }
  if ((*_NOM_Geo_Ptr)->city != NULL)
  {
    free((void*)(*_NOM_Geo_Ptr)->city);
    (*_NOM_Geo_Ptr)->city = NULL;
  }
  if ((*_NOM_Geo_Ptr)->county != NULL)
  {
    free((void*)(*_NOM_Geo_Ptr)->county);
    (*_NOM_Geo_Ptr)->county = NULL;
  }
  if ((*_NOM_Geo_Ptr)->postcode != NULL)
  {
    free((void*)(*_NOM_Geo_Ptr)->postcode);
    (*_NOM_Geo_Ptr)->postcode = NULL;
  }
  if ((*_NOM_Geo_Ptr)->street != NULL)
  {
    free((void*)(*_NOM_Geo_Ptr)->street);
    (*_NOM_Geo_Ptr)->street = NULL;
  }

  free(*_NOM_Geo_Ptr);

  *_NOM_Geo_Ptr = NULL;
  _NOM_Geo_Ptr = NULL;

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
