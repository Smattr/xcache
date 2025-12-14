#include "debug.h"
#include "inferior_t.h"
#include "io.h"
#include "list.h"
#include "output.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/// is an output redundant due to a previous action?
///
/// @param sub An earlier action that may be subsumed by `dom`
/// @param dom The current output we are processing
/// @return True if `dom` dominates `sub`
static bool is_dominated_by(const io_t sub, const output_t dom) {

  // `chmod;chmod` can be de-duped into just `chmod`
  if (sub.tag == IO_OUTPUT) {
    if (sub.output.tag == OUT_CHMOD && dom.tag == OUT_CHMOD) {
      assert(sub.output.chmod.mode == 0 &&
             "chmod output’s mode not set to placeholder");
      assert(dom.chmod.mode == 0 &&
             "chmod output’s mode not set to placeholder");
      if (strcmp(sub.output.path, dom.path) == 0) {
        DEBUG("dropping chmod of \"%s\" as output because a later chmod of it "
              "was seen",
              dom.path);
        return true;
      }
    }
  }

  // `write;write` can be de-duped into just `write`
  if (sub.tag == IO_OUTPUT) {
    if (sub.output.tag == OUT_WRITE && dom.tag == OUT_WRITE) {
      assert(sub.output.write.mode == 0 &&
             "write output’s mode is not set to placeholder");
      assert(dom.write.mode == 0 &&
             "write output’s mode is not set to placeholder");
      if (strcmp(sub.output.path, dom.path) == 0) {
        DEBUG("dropping write of \"%s\" as output because a later write of it "
              "was seen",
              dom.path);
        return true;
      }
    }
  }

  return false;
}

int inferior_output_new(inferior_t *inf, const output_t output) {

  assert(inf != NULL);

  int rc = 0;

  // we can drop any earlier output that is dominated by this output
  for (size_t i = 0; i < LIST_SIZE(&inf->io);) {
    io_t prior = *LIST_AT(&inf->io, i);
    if (is_dominated_by(prior, output)) {
      (void)LIST_POP(&inf->io, i);
      io_free(prior);
    } else {
      ++i;
    }
  }

  const io_t io = {.tag = IO_OUTPUT, .output = output};
  if (ERROR((rc = LIST_PUSH_BACK(&inf->io, io))))
    goto done;

done:
  return rc;
}
