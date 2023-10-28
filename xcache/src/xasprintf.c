#include "alloc.h"
#include <stdarg.h>
#include <stdio.h>

int xasprintf(char **restrict strp, const char *restrict fmt, ...) {

  va_list ap;
  va_start(ap, fmt);

  int rc = vasprintf(strp, fmt, ap);

  va_end(ap);

  return rc;
}
