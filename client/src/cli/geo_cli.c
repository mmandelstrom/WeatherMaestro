#include "cli/geo_cli.h"
#include "cli/cli.h"

#include "error.h"
#include "misc_utils.h"

#include <string.h>

Geo_Client Client;

/* ------------------------ CLI Command functions ------------------------ */

static int geo_cli_cmd_query(int _argc, const char** _argv);
static int geo_cli_cmd_list(int _argc, const char** _argv);
static int geo_cli_cmd_add(int _argc, const char** _argv);
static int geo_cli_cmd_rm(int _argc, const char** _argv);


static const CLI_Cmd Geo_Commands[] = {
  {
    "query",
    "Return list of saved geolocations",
    geo_cli_cmd_query,
    'q',
  },
  {
    "list",
    "Return list of saved geolocations",
    geo_cli_cmd_list,
    'l',
  },
  {
    "add",
    "Save a new geolocation to cache",
    geo_cli_cmd_add,
    'a',
  },
  {
    "remove",
    "Remove a saved geolocation from cache",
    geo_cli_cmd_rm,
    'r',
  },
  { NULL, NULL, NULL, 0 } // Sentinel command, marks array end
};

static const CLI_Opt Geo_Opts[] = {
  {
    "--name",
    "Specify location by name (query)",
    C_TYPE_STRING,
    &Client.name,
    'n',
  },
  {
    "--lat",
    "Specify location by latitude (must be used with longitude)",
    C_TYPE_FLOAT,
    &Client.lat,
    0,
  },
  {
    "--lon",
    "Specify location by longitude (must be used with latitude)",
    C_TYPE_FLOAT,
    &Client.lon,
    0,
  },
  {
    "--count",
    "Specify how many potential locations to retrieve from search (works only with query)",
    C_TYPE_UINT8,
    &Client.count,
    'c',

  }
};

static int geo_cli_cmd_query(int _argc, const char** _argv)
{

  return SUCCESS;
}
static int geo_cli_cmd_list(int _argc, const char** _argv)
{
  if (_argc < 1)
    return ERR_INVALID_ARG;
  
  Geo_Client Client;

  int i;
  double lat = 0, lon = 0;
  for (i = 0; i < _argc; i++)
  {
    if (strcmp(_argv[i], "--lat") == 0 && _argv[i+1] != NULL)
    {
      if (parse_string_to_double(_argv[i+1], &lat) == SUCCESS)
        Client.lat = (float)lat;
    }
    if (strcmp(_argv[i], "--lon") == 0 && _argv[i+1] != NULL)
    {
      if (parse_string_to_double(_argv[i+1], &lon) == SUCCESS)
        Client.lon = (float)lon;
    }
  }

  // if (lat >=
  
  return SUCCESS;
}
static int geo_cli_cmd_add(int _argc, const char** _argv)
{

  return SUCCESS;
}
static int geo_cli_cmd_rm(int _argc, const char** _argv)
{

  return SUCCESS;
}
/* ----------------------------------------------------------------------- */

int geo_cli_dispatch(int _argc, const char** _argv)
{
  return cli_cmd_dispatch(Geo_Commands, _argc, _argv);
}
