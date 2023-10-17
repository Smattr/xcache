#include "debug.h"
#include <stdio.h>
#include <xcache/debug.h>

FILE *xc_set_debug(FILE *stream) {
  FILE *old = xc_debug;
  xc_debug = stream;
  return old;
}
