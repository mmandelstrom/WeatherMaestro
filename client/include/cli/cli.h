/* =================================================================== */
/* ============================= CLI Tools =========================== */
/* =================================================================== */

#ifndef __CLI_TOOLS_H__
#define __CLI_TOOLS_H__

#include <stdint.h>

#include "types.h"

typedef int (*cli_cmd_func)(int argc, const char** argv);

typedef struct
{
  const char*   name;
  const char*   desc; // description/help

  CType         type;
  void*         var;  // Arg value

  const char    shrt; 

} CLI_Opt;

typedef struct
{
  const char*   name;
  const char*   desc; // description/help
  cli_cmd_func  func;

  const char    shrt; // shortname command, e.g "w" for "weather"

} CLI_Cmd;


int cli_cmd_dispatch(const CLI_Cmd* _Commands, int _argc, const char** _argv);
int cli_opt_dispatch(const CLI_Opt* _Options, int _argc, const char** _argv, void* _val);

/* Displays a help message when -h/--help added after command 
 * _cmd_name should be first argv */
void cli_cmd_print_help(const char* _cmd_name, const CLI_Cmd* _Commands);
void cli_opt_print_help(const char* _cmd_name, const CLI_Opt* _Options);


#endif
