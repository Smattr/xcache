#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
#include "list.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/// does one input make the other redundant?
///
/// @param sub The current input we are processing
/// @param dom An earlier input that may subsume `sub`
/// @return True if `dom` dominates `sub`
static bool is_dominated_by_input(const input_t sub, const input_t dom) {

  // `read;read` can be de-duped into just `read`
  if (sub.tag == INP_READ && dom.tag == INP_READ) {
    if (strcmp(sub.path, dom.path) == 0) {
      DEBUG("skipping read of \"%s\" as input because a read of it is already "
            "an input",
            sub.path);
      return true;
    }
  }

  // `readlink;readlink` can be de-duped into just `readlink`
  if (sub.tag == INP_READLINK && dom.tag == INP_READLINK) {
    if (strcmp(sub.path, dom.path) == 0) {
      DEBUG("skipping readlink of \"%s\" as input because a readlink of it is "
            "already an input",
            sub.path);
      return true;
    }
  }

  return false;
}

/// does an output make a later input?
///
/// @param sub The current input we are processing
/// @param dom An earlier output that may subsume `sub`
/// @return True if `dom` dominates `sub`
static bool is_dominated_by_output(const input_t sub, const output_t dom) {

  // A prior file write “dominates” a file read, in the sense that the read is
  // only consuming data the tracee already had, not new external input.This can
  // happen when e.g. the target writes out a temporary file and then reads it
  // back in.
  if (sub.tag == INP_READ && dom.tag == OUT_WRITE) {
    if (strcmp(sub.path, dom.path) == 0) {
      DEBUG("skipping read of \"%s\" as input because a write of it is "
            "already an output",
            sub.path);
      return true;
    }
  }

  return false;
}

int inferior_input_new(inferior_t *inf, const input_t input) {

  assert(inf != NULL);

  int rc = 0;

  // we can elide this input if it is dominated by an earlier output
  for (size_t i = LIST_SIZE(&inf->outputs) - 1; i != SIZE_MAX; --i) {
    const output_t prior = *LIST_AT(&inf->outputs, i);
    if (is_dominated_by_output(input, prior))
      goto done;
  }

  // we can elide this input if it is dominated by an earlier input
  for (size_t i = LIST_SIZE(&inf->inputs) - 1; i != SIZE_MAX; --i) {
    const input_t prior = *LIST_AT(&inf->inputs, i);
    if (is_dominated_by_input(input, prior))
      goto done;
  }

  if (ERROR((rc = LIST_PUSH_BACK(&inf->inputs, input))))
    goto done;

done:
  return rc;
}
