/* =================================================================== */
/* ============================= CLI Tools =========================== */
/* =================================================================== */

#ifndef __CLI_TOOLS_H__
#define __CLI_TOOLS_H__


typedef void (*cli_cmd_func)(int argc, const char** argv);

typedef struct
{
  const char*   name;
  const char    shrt; // shortname command, e.g "w" for "weather"
  cli_cmd_func  func;
  const char*   desc; // description/help

} CLI_Cmd;


/* Displays a help message when -h/--help added after command 
 * _cmd_name is simply first argv */
void cli_cmd_print_help(const char* _cmd_name, const CLI_Cmd* _Commands);

#endif
