#include "debug.h"
#include "inferior_t.h"
#include "input.h"
#include "io.h"
#include "list.h"
#include "output.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/// is an input redundant due to a previous action?
///
/// @param sub The current input we are processing
/// @param dom An earlier action that may subsume `sub`
/// @return True if `dom` dominates `sub`
static bool is_dominated_by(const input_t sub, const io_t dom) {

  // `access;access` with the same options can be de-duped into just `access`
  if (dom.tag == IO_INPUT) {
    if (sub.tag == INP_ACCESS && dom.input.tag == INP_ACCESS) {
      if (sub.access.mode == dom.input.access.mode) {
        if (sub.access.flags == dom.input.access.flags) {
          if (strcmp(sub.path, dom.input.path) == 0) {
            DEBUG(
                "skipping access of \"%s\" as input because an access of it is "
                "already an input",
                sub.path);
            return true;
          }
        }
      }
    }
  }

  // `read;read` can be de-duped into just `read`
  if (dom.tag == IO_INPUT) {
    if (sub.tag == INP_READ && dom.input.tag == INP_READ) {
      if (strcmp(sub.path, dom.input.path) == 0) {
        DEBUG(
            "skipping read of \"%s\" as input because a read of it is already "
            "an input",
            sub.path);
        return true;
      }
    }
  }

  // `readlink;readlink` can be de-duped into just `readlink`
  if (dom.tag == IO_INPUT) {
    if (sub.tag == INP_READLINK && dom.input.tag == INP_READLINK) {
      if (strcmp(sub.path, dom.input.path) == 0) {
        DEBUG(
            "skipping readlink of \"%s\" as input because a readlink of it is "
            "already an input",
            sub.path);
        return true;
      }
    }
  }

  // `getenv;getenv` can be de-duped into just `getenv`
  if (dom.tag == IO_INPUT) {
    if (sub.tag == INP_GETENV && dom.input.tag == INP_GETENV) {
      if (strcmp(sub.path, dom.input.path) == 0) {
        DEBUG("skipping getenv(\"%s\") as input because a getenv(\"%s\") of it "
              "is already an input",
              sub.path, sub.path);
        return true;
      }
    }
  }

  // A prior file write “dominates” a file read, in the sense that the read is
  // only consuming data the tracee already had, not new external input.This can
  // happen when e.g. the target writes out a temporary file and then reads it
  // back in.
  if (dom.tag == IO_OUTPUT) {
    if (sub.tag == INP_READ && dom.output.tag == OUT_WRITE) {
      if (strcmp(sub.path, dom.output.path) == 0) {
        DEBUG("skipping read of \"%s\" as input because a write of it is "
              "already an output",
              sub.path);
        return true;
      }
    }
  }

  return false;
}

int inferior_input_new(inferior_t *inf, input_t input) {

  assert(inf != NULL);

  int rc = 0;

  // we can elide this input if it is dominated by an earlier action
  for (size_t i = LIST_SIZE(&inf->io) - 1; i != SIZE_MAX; --i) {
    const io_t prior = *LIST_AT(&inf->io, i);
    if (is_dominated_by(input, prior)) {
      input_free(input);
      goto done;
    }
  }

  const io_t io = {.tag = IO_INPUT, .input = input};
  if (ERROR((rc = LIST_PUSH_BACK(&inf->io, io))))
    goto done;

done:
  return rc;
}
