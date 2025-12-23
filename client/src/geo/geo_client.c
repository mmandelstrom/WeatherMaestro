#include "geo/geo_client.h"

#include <stdio.h>

void geo_cli_dispatch(int argc, const char** argv)
{
  printf("args:\n");
  for (int i = 0; i < argc; i++)
    printf("%s\n", argv[i]);
}
