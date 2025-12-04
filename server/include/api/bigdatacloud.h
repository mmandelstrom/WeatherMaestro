
#ifndef __BIGDATACLOUD_H__
#define __BIGDATACLOUD_H__

/* ******************************************************************* */
/* ************************ BigDataCloud API ************************* */
/* ******************************************************************* */
/* Ahhh, good old open-meteo */

#include "../../../utils/include/http_client.h"
#include "../../../libs/include/cJSON.h"
#include "../../../utils/include/json_utils.h"
#include "../../../utils/include/curl.h"

#include <stdio.h>
#include <time.h>

/* #define BIGDATACLOUD_BASE_URL "https://api.bigdatacloud.net/data/%s" */
#define BIGDATACLOUD_REVERSE_GEOCODE_QUERY "https://api.bigdatacloud.net/data/reverse-geocode-client?latitude=%f&longitude=%f&localityLanguage=en"

typedef struct
{
  const char* country_name;
  const char* city;
  const char* locality;

  const char* timezone; // local timezone, ex: "Europe/Stockholm"
  
  float       latitude;
  float       longitude;

  char        country_code[3]; // two-char country code, ex: "SE"

} Bigdatacloud_Geo;

/* ---------------------- Interface ----------------------- */

int bigdatacloud_init_ptr(Bigdatacloud_Geo** _BDC_Location_Ptr);

int bigdatacloud_get_geo_by_coords(Bigdatacloud_Geo* _BDC_Location, float _lat, float _lon);

void bigdatacloud_dispose_ptr(Bigdatacloud_Geo** _BDC_Location_Ptr);

/* -------------------------------------------------------- */

#endif
