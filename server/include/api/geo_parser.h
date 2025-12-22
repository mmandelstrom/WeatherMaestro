#ifndef __GEO_PARSER_H__
#define __GEO_PARSER_H__

/* ******************************************************************* */
/* *********************** GEOLOCATION PARSER ************************ */
/* ******************************************************************* */

#include "api/bigdatacloud.h"
#include "api/nominatim.h"
#include "time_utils.h"
#include "file_utils.h"
#include "md5.h"

#define CACHE_DIR "data/cache/"
#define CITY_COORD_BUFFER_LENGTH 12 // How many max chars we take from latitude+longitude param strings. 12 will for example allow -111.111111
#define CITY_NAME_BUFFER_LENGTH 128 // How many max chars we take from name param string

typedef enum
{
  OPEN_METEO_GEO,
  NOMINATIM_OSM_GEO,
  BIGDATACLOUD_GEO,

} ExternalGeoAPI;

typedef enum
{
  GEO_PARSER_INITIALIZING,
  GEO_PARSER_CHECKING_CACHE,
  GEO_PARSER_CALLING_EXT_API,
  GEO_PARSER_IDLING,
  GEO_PARSER_PARSING,
  GEO_PARSER_RESPONDING,
  GEO_PARSER_DISPOSING,
  GEO_PARSER_ERROR

} GeoParserState;


typedef struct
{
  const char* country;
  const char* county;
  const char* city;
  const char* postcode;
  const char* street;
  const char* house_number;
  const char* locality; // Maybe only bigdatacloud specific
  const char* timezone; // local timezone, ex: "Europe/Stockholm"

  char        timezone_gmt[7]; // local timezone, ex: "GMT+12"
  
  float       lat;
  float       lon;

  char        country_code[3]; // two-char country code, ex: "SE"

} Geo_Values; // shouldn't be used directly by interfaces

typedef struct
{
  const char*     query;
  const char*     cache_path;

  Geo_Values**    values;
  int             count;

} Geo; // wrapper for Geo structs


typedef struct {
  Geo*              geo;
  const char*       query;
  Scheduler_Task*   task;
  char*             json_output;
  
  float             latitude;
  float             longitude;

  GeoParserState    state;

  bool              use_query;

} Geo_Parser;

/* ---------------------- Interface ----------------------- */

/** Heap init for multi Geo wrapper struct. Count decides how many Geo structs to init 
 * To init their weather/forecast structs, pass true as that arg */
int geo_parser_init_ptr(Geo_Parser** _Parser_Ptr, void* _context, bool use_query, const char* _query, float _lat, float _lon);
/** Pre-reqs: Geo must be inited */
int geo_parser_get_geo_by_query(Geo_Parser* _Parser);

/** Pre-reqs: Geo must be inited 
 * Will only return one geo no matter how many was inited in Geo */
int geo_parser_get_geo_by_coords(Geo_Parser* _Parser);

/** Helper for taking a string and trying to convert it to float
 * Only takes COORD_BUFFER_LENGTH amount of chars to target
 * Returns 1 if succesful, 0 if none parsed and -1 on error */
int geo_parser_lat_lon(const char* _val, float* _target_coord);

/** Heap dispose for data structs. To skip one, pass NULL as argument */
void geo_parser_dispose_ptr(Geo** _C_Ptr);

/* -------------------------------------------------------- */


#endif
