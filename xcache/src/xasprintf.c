#include "alloc.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void xasprintf(char **restrict strp, const char *restrict fmt, ...) {

  va_list ap;
  va_start(ap, fmt);

  int rc = vasprintf(strp, fmt, ap);
  if (rc < 0) {
    fprintf(stderr, "vasprintf: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  va_end(ap);
}