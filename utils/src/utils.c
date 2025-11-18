#define _POSIX_C_SOURCE 199309L

#include "../include/utils.h"
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif /*_WIN32*/

uint64_t SystemMonotonicMS() {
  long ms;
  time_t s;

  struct timespec spec;
  clock_gettime(CLOCK_MONOTONIC, &spec);
  s = spec.tv_sec;
  ms = (spec.tv_nsec / 1000000);

  uint64_t result = s;
  result *= 1000;
  result += ms;

  return result;
}

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

void ms_sleep(uint64_t ms) {
#ifdef _WIN32
    Sleep((DWORD)ms); 
#else
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

/* char* strdup(const char* _str)
{
  char *cpy = NULL;
  if (_str)
  {
    cpy = malloc(strlen(_str) + 1);
    if (cpy)
      strcpy(cpy, _str);
    else
      return NULL;
  }
  return cpy;
} */

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
