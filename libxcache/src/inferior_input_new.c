#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
#include "list.h"
#include <assert.h>
#include <string.h>

int inferior_input_new(inferior_t *inf, const input_t input) {

  assert(inf != NULL);

  // If this is already considered an output, ignore it. This can happen when
  // e.g. the target writes out a temporary file and then reads it back in.
  for (size_t i = 0; i < LIST_SIZE(&inf->outputs); ++i) {
    if (strcmp(LIST_AT(&inf->outputs, i)->path, input.path) == 0) {
      DEBUG("skipping \"%s\" as input because it is already an output",
            input.path);
      return 0;
    }
  }

  int rc = 0;

  if (ERROR((rc = LIST_PUSH_BACK(&inf->inputs, input))))
    goto done;

done:
  return rc;
}
