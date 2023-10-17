#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *xstrdup(const char *s) {
  char *copy = strdup(s);
  if (copy == NULL) {
    fputs("out of memory\n", stderr);
    exit(EXIT_FAILURE);
  }
  return copy;
}
