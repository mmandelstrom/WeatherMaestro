#ifndef __UTILS_H__
#define __UTILS_H__
#include <stdint.h>

uint64_t SystemMonotonicMS();
void ms_sleep(uint64_t ms);
char* stringcat(const char* _a, const char* _b);
char* strdup(const char* _str);

#endif
