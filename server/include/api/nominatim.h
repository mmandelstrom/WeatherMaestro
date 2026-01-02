
#ifndef __NOMINATIM_H__
#define __NOMINATIM_H__

/* ******************************************************************* */
/* ************************** Nominatim API ************************** */
/* ******************************************************************* */

#include "cJSON.h"
#include "http_client.h"
#include "json_utils.h"
#include "misc_utils.h"
#include "curl.h"

#include <stdio.h>


#define NOMINATIM_GEOCODE_QUERY "http://nominatim.openstreetmap.org/search?format=json&addressdetails=1&q=%s"


typedef struct
{
  const char* display_name;
  const char* country;
  const char* county;
  const char* city;
  const char* postcode;
  const char* road;
  const char* house_number;

  /* const char* timezone; // local timezone, ex: "Europe/Stockholm" */
  
  double      lat;
  double      lon;

  char        country_code[3]; // two-char country code, ex: "SE"

} Nominatim_Geo;

typedef struct {

  Nominatim_Geo* geo;

  int count;

} Nominatim_Result;



typedef void (*on_ext_api_finish)(void* _context, void* _ext_api);

typedef struct {

  on_ext_api_finish on_finish;
  void*             context;
  HTTP_Client*      http_client;
  char*             http_response;
  Nominatim_Result* result;
  char*             query;

  float             lat;
  float             lon;

  bool              use_query;
  
} Nominatim;

/* ---------------------- Interface ----------------------- */
int nominatim_get_geo(Nominatim** _NOM, bool _use_query, const char* _query, float _lat, float _lon, on_ext_api_finish _on_finish, void* _context);

int nominatim_init_ptr(Nominatim_Geo** _NOM_Geo_Ptr);
/* *_geo_count will return how many Nominatim_Geo structs will be in pointer */
int nominatim_get_geo_by_query(Nominatim_Geo** _NOM_Geo_Ptr, int* _geo_count, const char* _query);
// int nominatim_get_geo_by_coords(Nominatim_Geo* _NOM_Geo, int* _geo_count, float _lat, float _lon);

void nominatim_dispose(Nominatim** _N_Ptr);
void nominatim_dispose_ptr(Nominatim_Geo** _NOM_Geo_Ptr, int _count);

/* -------------------------------------------------------- */

#endif
