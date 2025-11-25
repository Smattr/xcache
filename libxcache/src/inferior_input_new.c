#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
#include "list.h"
#include <assert.h>
#include <string.h>

int inferior_input_new(inferior_t *inf, const input_t input) {

  assert(inf != NULL);

  // A prior file write “dominates” a file read, in the sense that the read is
  // only consuming data the tracee already had, not new external input.This can
  // happen when e.g. the target writes out a temporary file and then reads it
  // back in.
  if (input.tag == INP_READ) {
    for (size_t i = 0; i < LIST_SIZE(&inf->outputs); ++i) {
      const output_t *const output = LIST_AT(&inf->outputs, i);
      if (output->tag != OUT_WRITE)
        continue;
      if (strcmp(output->path, input.path) == 0) {
        DEBUG("skipping read of \"%s\" as input because a write of it is "
              "already an output",
              input.path);
        return 0;
      }
    }
  }

  int rc = 0;

  if (ERROR((rc = LIST_PUSH_BACK(&inf->inputs, input))))
    goto done;

done:
  return rc;
}
