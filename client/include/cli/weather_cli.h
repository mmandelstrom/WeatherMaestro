/* =================================================================== */
/* =========================== Weather CLI =========================== */
/* =================================================================== */

#ifndef __WEATHER_CLIENT_H__
#define __WEATHER_CLIENT_H__


#define WEATHER_CACHE_DIR           "data/cache/weather/"
#define WEATHER_CURRENT_CACHE_DIR   "data/cache/weather/current/"
#define WEATHER_FORECAST_CACHE_DIR  "data/cache/weather/forecast/"


int weather_cli_dispatch(int argc, const char** argv);


#endif
