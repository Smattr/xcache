#include "output_t.h"
#include <stdlib.h>

void output_free(output_t output) {
  free(output.path);
  free(output.cached_copy);
}
