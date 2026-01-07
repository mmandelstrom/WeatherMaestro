#include "cli/maestro_cli.h"

#include "cli/cli.h"
#include "cli/geo_cli.h"
#include "cli/weather_cli.h"
#include "error.h"

#include <stdio.h>
#include <string.h>


static int cli_cmd_version(int _argc, const char** _argv);
static int cli_cmd_weather(int _argc, const char** _argv);
static int cli_cmd_forecast(int _argc, const char** _argv);
static int cli_cmd_geo(int _argc, const char** _argv);


/* ------------------------ CLI Command functions ------------------------ */

static const CLI_Cmd Maestro_Commands[] = {
  {
    "weather",
    "Get/Show current weather information",
    cli_cmd_weather,
    'w',
  },
  {
    "forecast",
    "Get/Show future forecast information",
    cli_cmd_weather,
    'f',
  },
  {
    "geo",
    "Get/Show geolocation data",
    cli_cmd_geo,
    'g',
  },
  {
    "version",
    "Show program version",
    cli_cmd_version,
    'v',
  },
  { NULL, NULL, NULL, 0 } // Sentinel command, marks array end
};


static int cli_cmd_version(int _argc, const char** _argv) 
{
  printf("%s\n%s\nVersion: %s\n", LOGO, CLIENT_NAME, CLIENT_VERSION);
  return SUCCESS;
}
static int cli_cmd_weather(int _argc, const char** _argv) 
{
  return weather_cli_dispatch(_argc - 1, _argv + 1);
}
static int cli_cmd_forecast(int _argc, const char** _argv) 
{
  return weather_cli_dispatch(_argc - 1, _argv + 1);
}
static int cli_cmd_geo(int _argc, const char** _argv) 
{
  return geo_cli_dispatch(_argc - 1, _argv + 1);
}

/* ----------------------------------------------------------------------- */

int maestro_cli_dispatch(int _argc, const char** _argv)
{
  if (cli_cmd_dispatch(Maestro_Commands, _argc, _argv) != SUCCESS)
    cli_cmd_print_help(_argv[0], Maestro_Commands);

  return SUCCESS;
}
