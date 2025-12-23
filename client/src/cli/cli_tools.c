#include "cli/cli_tools.h"

#include <stdio.h>
#include <string.h>


void cli_cmd_print_help(const char* _cmd_name, const CLI_Cmd* _Commands)
{
  if (_cmd_name == NULL ||
      _Commands == NULL)
    return;

  printf("Usage: %s [COMMAND]\n", _cmd_name);
  printf("\nCommands:\n");
  int i;
  for (i = 0; _Commands[i].name; i++)
    printf("  %-12s %s\n", _Commands[i].name, _Commands[i].desc);

  printf("\nGlobal:\n");
  printf("  -h, --help     Show this help\n");
  printf("  --version      Show version\n\n");
}
