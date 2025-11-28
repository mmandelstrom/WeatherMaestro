#define _POSIX_C_SOURCE 199309L

#include "../include/misc_utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Concatenates two strings using malloc, strcpy and strcat
 * (goes without saying but needs to be freed by caller) */
char* stringcat(const char* _a, const char* _b)
{
  size_t len = strlen(_a) + strlen(_b) + 1;
  char* ab = (char*)malloc(sizeof(char) * len);
  strcpy(ab, _a);
  strcat(ab, _b);

  return ab;
}

#ifndef strdup
/* C89 approved strdup */
char* strdup(const char* _str) {
  if (_str == NULL)
    return NULL;

  size_t size = strlen(_str) + 1;
  char* copy = malloc(size);
  if (copy)
    strcpy(copy, _str);
  else
    return NULL;

  return copy;
}
#endif /* strdup */
