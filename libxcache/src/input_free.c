#include "input.h"
#include <stdlib.h>

void input_free(input_t i) {
  free(i.path);

  if (i.tag == INP_GETENV)
    free(i.getenv.value);
}
