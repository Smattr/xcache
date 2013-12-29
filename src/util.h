#ifndef _XCACHE_UTIL_H_
#define _XCACHE_UTIL_H_

#include <stdlib.h>

char *abspath(const char *relpath);

unsigned int hash(size_t limit, const char *s);

#endif
