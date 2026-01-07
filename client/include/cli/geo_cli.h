/* =================================================================== */
/* =========================== GEOLOCATION =========================== */
/* =================================================================== */

#ifndef __GEO_CLIENT_H__
#define __GEO_CLIENT_H__

#include "stdint.h"

#define GEO_CACHE_DIR "data/cache/geo/"

typedef struct
{
  char*       name;
  float       lat;
  float       lon;
  uint8_t     count;

} Geo_Client;

int geo_cli_dispatch(int argc, const char** argv);


#endif
