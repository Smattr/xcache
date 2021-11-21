#include "debug.h"
#include <stdio.h>
#include <xcache/debug.h>

void xc_set_debug(FILE *stream) { debug = stream; }
