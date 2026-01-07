#include "cli/cli.h"
#include "error.h"

#include <stdio.h>
#include <string.h>

void cli_cmd_print_help(const char* _cmd_name, const CLI_Cmd* _Commands);
void cli_opt_print_help(const char* _cmd_name, const CLI_Opt* _Options);

int cli_cmd_dispatch(const CLI_Cmd* _Commands, int _argc, const char** _argv)
{
  if (_argc < 2)
    return ERR_INVALID_ARG;

  const char* cmd = _argv[1];

  if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
    cli_cmd_print_help(_argv[0], _Commands);
    return SUCCESS;
  }  

  int i;
  for (i = 0; _Commands[i].name; i++)
  {
    if (strcmp(cmd, _Commands[i].name) == 0 ||
        (cmd[0] == _Commands[i].shrt && 
         cmd[1] == '\0'))
    {
      _Commands[i].func(_argc, _argv);
      return SUCCESS;
    }
  }

  return ERR_INVALID_ARG;
}

int cli_opt_dispatch(const CLI_Opt* _Options, int _argc, const char** _argv, void* _val)
{
  if (_argc < 2)
  {
    cli_opt_print_help(_argv[0], _Options);
    return ERR_INVALID_ARG;
  }

  const char* cmd = _argv[1];

  if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
    cli_opt_print_help(_argv[0], _Options);
    return SUCCESS;
  }  

  int i;
  for (i = 0; _Options[i].name; i++)
  {
    if (strcmp(cmd, _Options[i].name) == 0 ||
        (cmd[0] == '-' && 
         cmd[1] == _Options[i].shrt && 
         cmd[2] == '\0'))
    {
      // _Options[i].func(_argc, _argv);
      return SUCCESS;
    }
  }

  printf("Invalid argument...\n");

  cli_opt_print_help(_argv[0], _Options);
  return ERR_INVALID_ARG;
}

/* Displays a help message when -h/--help added after command 
 * _cmd_name should be first argv */
void cli_cmd_print_help(const char* _cmd_name, const CLI_Cmd* _Commands)
{
  if (_cmd_name == NULL ||
      _Commands == NULL)
    return;

  printf("Usage: %s [COMMAND]\n", _cmd_name);
  printf("\nCommands:\n");
  int i;
  for (i = 0; _Commands[i].name; i++)
  {
    printf("  %-10s", _Commands[i].name); 
    if (_Commands[i].shrt != 0) printf(" -%c", _Commands[i].shrt);   
    printf("  %s\n", _Commands[i].desc);
  }

  printf("\nGlobal:\n");
  printf("  -h, --help     Show this help\n");
}

void cli_opt_print_help(const char* _cmd_name, const CLI_Opt* _Options)
{
  if (_cmd_name == NULL ||
      _Options == NULL)
    return;

  printf("Usage: %s [OPTIONS]\n", _cmd_name);
  printf("\nOptions:\n");
  int i;
  for (i = 0; _Options[i].name; i++)
  {
    printf("  %-10s", _Options[i].name); 
    if (_Options[i].shrt != 0) printf(" -%c", _Options[i].shrt);   
    printf("  %s\n", _Options[i].desc);
  }

}
