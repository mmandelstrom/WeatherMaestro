#ifndef __GEO_PARSER_H__
#define __GEO_PARSER_H__

/* ******************************************************************* */
/* *********************** GEOLOCATION PARSER ************************ */
/* ******************************************************************* */

#include "weather_parser.h"

#define CACHE_DIR "data/cache/"
#define CITY_COORD_BUFFER_LENGTH 12 // How many max chars we take from latitude+longitude param strings. 12 will for example allow -111.111111
#define CITY_NAME_BUFFER_LENGTH 128 // How many max chars we take from name param string

typedef enum
{
  OPEN_METEO_GEO,
  NOMINATIM_OSM_GEO,
  BIGDATACLOUD_GEO,

} ExternalGeoAPI;

typedef struct
{
  Weather*    weather;
  Forecast*   forecast;

  const char* country;
  const char* county;
  const char* city;
  const char* postcode;
  const char* street;
  const char* locality;
  const char* timezone; // local timezone, ex: "Europe/Stockholm"

  char        timezone_gmt[7]; // local timezone, ex: "GMT+12"
  
  float       lat;
  float       lon;

  int         street_number;

  char        country_code[3]; // two-char country code, ex: "SE"

} Geo; // shouldn't be used directly by interfaces

typedef struct
{
  const char* query;
  const char* cache_path;

  Geo**  geo;
  int   count;

} Geos; // wrapper for Geo structs


/* ---------------------- Interface ----------------------- */

/** Heap init for multi Geos wrapper struct. Count decides how many Geo structs to init 
 * To init their weather/forecast structs, pass true as that arg */
int geo_parser_init_geos_ptr(Geos** _Gs_Ptr, int _count, bool _weather, bool _forecast);

/** Pre-reqs: Geos must be inited
 * Can retrieve more than one geolocation, maximum is defined by Geos->count */
int geo_parser_get_geo_by_query(Geos* _Geos, const char* _query, char* _json_output);
/** Pre-reqs: Geo must be inited  */
int geo_parser_get_geo_by_coords(Geos* _Geos, float _lat, float _lon, char* _json_output);

/** Helper for taking a string and trying to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful, 0 if none parsed and -1 on error */
int geo_parser_lat_lon(const char* _val, float* _target_coord);

/** Heap dispose for data structs. To skip one, pass NULL as argument */
void geo_parser_dispose_ptr(Geos** _C_Ptr, Weather** _W_Ptr, Forecast** _F_Ptr);

/* -------------------------------------------------------- */


#endif
