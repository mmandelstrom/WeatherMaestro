#include "cli/cli_client.h"

#include "error.h"

#include <stdio.h>
#include <string.h>


const char* logo = \
" _  _  ____   __  ____  _  _  ____  ____  \n"
"/ )( \\(  __) / _\\(_  _)/ )( \\(  __)(  _ \\ \n"
"\\ /\\ / ) _) /    \\ )(  ) __ ( ) _)  )   / \n"
"(_/\\_)(____)\\_/\\_/(__) \\_)(_/(____)(__\\_) \n"
" _  _   __   ____  ____  ____  ____   __  \n"
"( \\/ ) / _\\ (  __)/ ___)(_  _)(  _ \\ /  \\ \n"
"/ \\/ \\/    \\ ) _) \\___ \\  )(   )   /(  O )\n"
"\\_)(_/\\_/\\_/(____)(____/ (__) (__\\_) \\__/ \n";


static void cli_cmd_version(int _argc, const char** _argv);
static void cli_cmd_weather(int _argc, const char** _argv);
static void cli_cmd_geo(int _argc, const char** _argv);


/* ------------------------ CLI Command functions ------------------------ */

static const CLI_Cmd CLI_Commands[] = {
  {
    "weather",
    'w',
    cli_cmd_weather,
    "weather [OPTIONS] - Get/Show weather data",
  },
  {
    "geo",
    'g',
    cli_cmd_geo,
    "geo [OPTIONS]     - Get/Show geolocation data"
  },
  {
    "version",
    'v',
    cli_cmd_version,
    "version           - Show program version"
  },
  { NULL, 0, NULL, NULL } // Sentinel command, marks array end
};


static void cli_cmd_version(int _argc, const char** _argv) {
  printf("%s\n%s\nVersion: %s\n", logo, CLIENT_NAME, CLIENT_VERSION);
}
static void cli_cmd_weather(int _argc, const char** _argv) {
  weather_cli_dispatch(_argc - 1, _argv + 1);
}
static void cli_cmd_geo(int _argc, const char** _argv) {
  geo_cli_dispatch(_argc - 1, _argv + 1);
}

/* ----------------------------------------------------------------------- */

int cli_dispatch(int _argc, const char** _argv)
{
  if (_argc < 2)
  {
    cli_cmd_print_help(_argv[0], CLI_Commands);
    return ERR_INVALID_ARG;
  }

  const char* cmd = _argv[1];

  if (strcmp(cmd, "--version") == 0) {
      cli_cmd_version(0, NULL);
      return SUCCESS;
  }

  int i;
  for (i = 0; CLI_Commands[i].name; i++)
  {
    if (strcmp(cmd, CLI_Commands[i].name) == 0 ||
        (cmd[0] == CLI_Commands[i].shrt && cmd[1] == '\0'))
    {
      CLI_Commands[i].func(_argc, _argv);
      return SUCCESS;
    }
  }

  cli_cmd_print_help(_argv[0], CLI_Commands);
  return ERR_INVALID_ARG;
}
