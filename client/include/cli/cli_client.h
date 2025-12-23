/* =================================================================== */
/* ============================ CLI Client =========================== */
/* =================================================================== */

#ifndef __CLI_H__
#define __CLI_H__

#define CLIENT_NAME "WeatherMaestro CLI Client"
#define CLIENT_VERSION "1.0.0"

#include "cli/cli_tools.h"
#include "geo/geo_client.h"
#include "weather/weather_client.h"

int cli_dispatch(int _argc, const char** _argv);

#endif
