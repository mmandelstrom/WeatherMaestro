
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


#define NOMINATIM_GEOCODE_QUERY "https://nominatim.openstreetmap.org/search?format=json&addressdetails=1&q=%s"


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

/* ---------------------- Interface ----------------------- */

int nominatim_init_ptr(Nominatim_Geo** _NOM_Geo_Ptr);

/* *_geo_count will return how many Nominatim_Geo structs will be in pointer */
int nominatim_get_geo_by_query(Nominatim_Geo** _NOM_Geo_Ptr, int* _geo_count, const char* _query);
// int nominatim_get_geo_by_coords(Nominatim_Geo* _NOM_Geo, int* _geo_count, float _lat, float _lon);

void nominatim_dispose_ptr(Nominatim_Geo** _NOM_Geo_Ptr, int _count);

/* -------------------------------------------------------- */

#endif
