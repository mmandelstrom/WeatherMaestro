#define _POSIX_C_SOURCE 199309L

#include "misc_utils.h"
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

int parse_string_to_double(const char* _str, double* _double)
{
  if (!_str || !_double)
    return -1;

  int read_max_len = 36; // precision; max chars to read
  char buf[read_max_len];
  int str_len = strlen(_str);

  errno = 0; // reset errno
  if (str_len > read_max_len)
  {
    strncpy(buf, _str, read_max_len);
    buf[read_max_len-1] = '\0';
  } 
  else 
  {
    strncpy(buf, _str, str_len);
    buf[str_len] = '\0';
  }

  char* endptr;
  double dubb = strtod(buf, &endptr);

  if (errno != 0 || endptr == buf || *endptr != '\0')
    return -1;

  *_double = dubb;

  return 0;
}

int parse_string_to_int(const char* _str, int* _int)
{
  if (!_str || !_int)
    return -1;

  int read_max_len = 36; // precision; max chars to read
  char buf[read_max_len];
  int str_len = strlen(_str);

  errno = 0; // reset errno
  if (str_len > read_max_len)
  {
    strncpy(buf, _str, read_max_len);
    buf[read_max_len-1] = '\0';
  } 
  else 
  {
    strncpy(buf, _str, str_len);
    buf[str_len] = '\0';
  }

  char* endptr;
  long val = strtol(buf, &endptr, 10); // base 10 integer

  if (errno != 0 || endptr == buf || *endptr != '\0')
    return -1;

  *_int = (int)val;

  return 0;
}
