#include "cli/weather_cli.h"

#include "cli/cli.h"
#include "error.h"

#include <string.h>

/* ------------------------ CLI Command functions ------------------------ */

static int weather_cli_cmd_current(int _argc, const char** _argv);
static int weather_cli_cmd_forecast(int _argc, const char** _argv);
// static int weather_cli_cmd(int _argc, const char** _argv);

static const CLI_Cmd Weather_Commands[] = {
  {
    "current",
    "Get current weather",
    weather_cli_cmd_current,
    'c',
  },
  {
    "forecast",
    "Get forecast weather",
    weather_cli_cmd_forecast,
    'f',
  },
  { NULL, NULL, NULL, 0 } // Sentinel command, marks array end
};

static int weather_cli_cmd_current(int _argc, const char** _argv)
{

  return SUCCESS;
}
static int weather_cli_cmd_forecast(int _argc, const char** _argv)
{

  return SUCCESS;
}
/* ----------------------------------------------------------------------- */

int weather_cli_dispatch(int _argc, const char** _argv)
{
  return cli_cmd_dispatch(Weather_Commands, _argc, _argv);
}
