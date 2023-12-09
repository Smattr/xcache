#include "output_t.h"
#include <stdlib.h>

void output_free(output_t output) {
  free(output.path);

  if (output.tag == OUT_WRITE)
    free(output.write.cached_copy);
}
