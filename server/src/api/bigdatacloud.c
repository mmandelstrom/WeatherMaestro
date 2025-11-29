#include "../../include/api/bigdatacloud.h"


/* ---------------------- Internal functions ----------------------- */

const char* bigdatacloud_get_reverse_geocode_json(float _lat, float _lon);
int bigdatacloud_parse_reverse_geocode_json(Bigdatacloud_Geo* _BDC_Geo, const char* _json);

/* ----------------------------------------------------------------- */

int bigdatacloud_init_ptr(Bigdatacloud_Geo** _BDC_Geo_Ptr)
{

  if (_BDC_Geo_Ptr == NULL)
    return -1;

  *_BDC_Geo_Ptr = malloc(sizeof(Bigdatacloud_Geo));
  if (*_BDC_Geo_Ptr == NULL)
  {
    perror("malloc");
    return -2;
  }

  memset(*_BDC_Geo_Ptr, 0, sizeof(Bigdatacloud_Geo));

  return 0;
}

int bigdatacloud_get_geo_by_coords(Bigdatacloud_Geo* _BDC_Geo, float _lat, float _lon)
{
  /* GET bigdatacloud city json */
  const char* bigdatacloud_json = bigdatacloud_get_reverse_geocode_json(_lat, _lon);
  if (bigdatacloud_json == NULL)
  {
    perror("bigdatacloud_get_weather_json");
    return -1;
  }

  /* int json_len = strlen(bigdatacloud_json);
  for (int i = 0; i < json_len; i++)
    printf("bigdatacloud_json[%i]: %i\n", i, (int)bigdatacloud_json[i]);
  printf("bigdatacloud_json: %s\n\n", bigdatacloud_json); */

  /* Parse bigdatacloud json to Bigdatacloud_Geo struct */
  int result = bigdatacloud_parse_reverse_geocode_json(_BDC_Geo, bigdatacloud_json);
  if (result != 0)
  {
    perror("bigdatacloud_parse_json");
    free((void*)bigdatacloud_json);
    return result;
  }
  free((void*)bigdatacloud_json);

  return 0;
}

const char* bigdatacloud_get_reverse_geocode_json(float _lat, float _lon)
{
  char url[256];

  int url_len = snprintf(url, 256, 
                         BIGDATACLOUD_REVERSE_GEOCODE_QUERY,
                         _lat, 
                         _lon);
  url[url_len] = '\0';
  
	Curl_Data C_Data;
	if (curl_init(&C_Data) != 0)
		return NULL;

  printf("\n--- CALLING BIGDATACLOUD RESPONSE --- \n\n");
    
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

  printf("===== Meteo Response JSON =====\n\n%s\n\n", response);

  return response;
}

int bigdatacloud_parse_reverse_geocode_json(Bigdatacloud_Geo* _BDC_Geo, const char* _json)
{
  cJSON* Json_Root = cJSON_Parse(_json);
  if (Json_Root == NULL) {
    const char* error_pointer = cJSON_GetErrorPtr();
    if (error_pointer != NULL){
      fprintf(stderr,"bigdatacloud json error %s\n", error_pointer);
    }
    return -1;
  }

  /* Parse data from json 
     "countryName": "Sweden",
  "countryCode": "SE",
  "locality": "Allmanna sjukhuset",
  "city": "Malmo",

  "informative"[1]: 
        "name": "Europe/Stockholm",
*/
  memcpy(_BDC_Geo->country_code, json_get_string(Json_Root, "countryCode"), 2);
  _BDC_Geo->country_code[2] = '\0';

  _BDC_Geo->latitude       = json_get_double(Json_Root, "latitude");
  _BDC_Geo->longitude      = json_get_double(Json_Root, "longitude");


  /* Heap allocations */
  _BDC_Geo->country_name   = strdup(json_get_string(Json_Root, "countryName"));
  _BDC_Geo->city           = strdup(json_get_string(Json_Root, "city"));
  _BDC_Geo->locality       = strdup(json_get_string(Json_Root, "locality"));

  cJSON* Json_Informative = cJSON_GetObjectItemCaseSensitive(Json_Root, "informative");
  if (Json_Informative != NULL)
  {
    cJSON* Json_Timezone = cJSON_GetArrayItem(Json_Informative, 1);
    _BDC_Geo->timezone = strdup(json_get_string(Json_Timezone, "name"));
  }

  if (_BDC_Geo->country_name  == NULL ||
      _BDC_Geo->city          == NULL)
  {
    fprintf(stderr, "One or more strings couldn't be parsed from bigdatacloud json\n");
    cJSON_Delete(Json_Root);
    return -4;
  }

  cJSON_Delete(Json_Root);

  return 0;
}

void bigdatacloud_dispose_ptr(Bigdatacloud_Geo** _BDC_Geo_Ptr)
{

  if (_BDC_Geo_Ptr == NULL)
    return;

  if (*_BDC_Geo_Ptr == NULL)
  {
    _BDC_Geo_Ptr = NULL;
    return;
  }

  if ((*_BDC_Geo_Ptr)->country_name != NULL)
  {
    free((void*)(*_BDC_Geo_Ptr)->country_name);
    (*_BDC_Geo_Ptr)->country_name = NULL;
  }
  if ((*_BDC_Geo_Ptr)->city != NULL)
  {
    free((void*)(*_BDC_Geo_Ptr)->city);
    (*_BDC_Geo_Ptr)->city = NULL;
  }
  if ((*_BDC_Geo_Ptr)->locality != NULL)
  {
    free((void*)(*_BDC_Geo_Ptr)->locality);
    (*_BDC_Geo_Ptr)->locality = NULL;
  }

  free(*_BDC_Geo_Ptr);

  *_BDC_Geo_Ptr = NULL;
  _BDC_Geo_Ptr = NULL;

}

/*
Example response:

{
  "latitude": 55.5836011,
  "lookupSource": "coordinates",
  "longitude": 13.0027658,
  "localityLanguageRequested": "en",
  "continent": "Europe",
  "continentCode": "EU",
  "countryName": "Sweden",
  "countryCode": "SE",
  "principalSubdivision": "Skane County",
  "principalSubdivisionCode": "SE-M",
  "city": "Malmo",
  "locality": "Allmanna sjukhuset",
  "postcode": "",
  "plusCode": "9F7MH2M3+C4",
  "localityInfo": {
    "administrative": [
      {
        "name": "Sweden",
        "description": "country in Northern Europe",
        "isoName": "Sweden",
        "order": 3,
        "adminLevel": 2,
        "isoCode": "SE",
        "wikidataId": "Q34",
        "geonameId": 2661886
      },
      {
        "name": "South Sweden",
        "order": 7,
        "adminLevel": 3
      },
      {
        "name": "Skane County",
        "description": "county (län) in Sweden",
        "isoName": "Skane County",
        "order": 8,
        "adminLevel": 4,
        "isoCode": "SE-M",
        "wikidataId": "Q103659",
        "geonameId": 3337385
      },
      {
        "name": "Malmo",
        "description": "city in Skåne County, Sweden",
        "order": 9,
        "adminLevel": 7,
        "wikidataId": "Q2211",
        "geonameId": 2692969
      },
      {
        "name": "Malmo Municipality",
        "description": "municipality in Skåne County, Sweden",
        "order": 10,
        "adminLevel": 7,
        "wikidataId": "Q503361",
        "geonameId": 2692965
      },
      {
        "name": "Soder",
        "description": "Location district in Scania, Sweden",
        "order": 11,
        "adminLevel": 9,
        "wikidataId": "Q15256329"
      },
      {
        "name": "Innerstaden",
        "description": "Malmö",
        "order": 12,
        "adminLevel": 9,
        "wikidataId": "Q15256332"
      },
      {
        "name": "Heleneholm",
        "description": "neighbourhood in Skåne County, Skåne, Sweden",
        "order": 13,
        "adminLevel": 10,
        "wikidataId": "Q5703924"
      },
      {
        "name": "Flensburg",
        "description": "neighbourhood in Skåne County, Skåne, Sweden",
        "order": 14,
        "adminLevel": 10,
        "wikidataId": "Q5458665"
      },
      {
        "name": "Sodervarn",
        "description": "neighbourhood in Skåne County, Skåne, Sweden",
        "order": 15,
        "adminLevel": 10,
        "wikidataId": "Q7666357",
        "geonameId": 2676164
      },
      {
        "name": "Allmanna sjukhuset",
        "order": 16,
        "adminLevel": 10,
        "wikidataId": "Q15734178"
      }
    ],
    "informative": [
      {
        "name": "Europe",
        "description": "continent in the Northern Hemisphere",
        "isoName": "Europe",
        "order": 1,
        "isoCode": "EU",
        "wikidataId": "Q46",
        "geonameId": 6255148
      },
      {
        "name": "Europe/Stockholm",
        "description": "time zone",
        "order": 2
      },
      {
        "name": "Nordic countries",
        "description": "geographical and cultural region in Northern Europe and the North Atlantic",
        "order": 4,
        "wikidataId": "Q52062",
        "geonameId": 2616167
      },
      {
        "name": "Scandinavia",
        "description": "region in Northern Europe",
        "order": 5,
        "wikidataId": "Q21195",
        "geonameId": 2614165
      },
      {
        "name": "Gotaland",
        "description": "southernmost land of Sweden",
        "order": 6,
        "wikidataId": "Q201694"
      }
    ]
  }
}

*/
