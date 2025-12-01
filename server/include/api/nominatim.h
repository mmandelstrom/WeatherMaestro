
#ifndef __NOMINATIM_H__
#define __NOMINATIM_H__

/* ******************************************************************* */
/* ************************** Nominatim API ************************** */
/* ******************************************************************* */

#include "../http/http_client.h"
#include "../../../libs/include/cJSON.h"
#include "../../../utils/include/json_utils.h"
#include "../../../utils/include/misc_utils.h"
#include "../../../utils/include/curl.h"

#include <stdio.h>


#define NOMINATIM_GEOCODE_QUERY "https://nominatim.openstreetmap.org/search?format=json&addressdetails=1&limit=1&q=%s"


typedef struct
{
  const char* country;
  const char* county;
  const char* city;
  const char* postcode;
  const char* street;

  /* const char* timezone; // local timezone, ex: "Europe/Stockholm" */
  
  double      lat;
  double      lon;

  int         house_number;

  char        country_code[3]; // two-char country code, ex: "SE"

} Nominatim_Geo;

/* ---------------------- Interface ----------------------- */

int nominatim_init_ptr(Nominatim_Geo** _NOM_Location_Ptr);

int nominatim_get_geo_by_query(Nominatim_Geo* _NOM_Location, const char* _query);

void nominatim_dispose_ptr(Nominatim_Geo** _NOM_Location_Ptr);

/* -------------------------------------------------------- */

#endif
