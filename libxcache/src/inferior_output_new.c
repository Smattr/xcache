#include "debug.h"
#include "inferior_t.h"
#include "list.h"
#include "output_t.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool is_dominated_by(const output_t sub, const output_t dom) {

  // `chmod;chmod` can be de-duped into just `chmod`
  if (sub.tag == OUT_CHMOD && dom.tag == OUT_CHMOD) {
    assert(sub.chmod.mode == 0 && "chmod output’s mode not set to placeholder");
    assert(dom.chmod.mode == 0 && "chmod output’s mode not set to placeholder");
    if (strcmp(sub.path, dom.path) == 0) {
      DEBUG("dropping chmod of \"%s\" as output because a later chmod of it "
            "was seen",
            sub.path);
      return true;
    }
  }

  // `write;write` can be de-duped into just `write`
  if (sub.tag == OUT_WRITE && dom.tag == OUT_WRITE) {
    assert(sub.write.mode == 0 &&
           "write output’s mode is not set to placeholder");
    assert(dom.write.mode == 0 &&
           "write output’s mode is not set to placeholder");
    if (strcmp(sub.path, dom.path) == 0) {
      DEBUG("dropping write of \"%s\" as output because a later write of it "
            "was seen",
            sub.path);
      return true;
    }
  }

  return false;
}

int inferior_output_new(inferior_t *inf, const output_t output) {

  assert(inf != NULL);

  int rc = 0;

  // we can drop any earlier output that is dominated by this output
  for (size_t i = 0; i < LIST_SIZE(&inf->outputs);) {
    output_t prior = *LIST_AT(&inf->outputs, i);
    if (is_dominated_by(prior, output)) {
      (void)LIST_POP(&inf->outputs, i);
      output_free(prior);
    } else {
      ++i;
    }
  }

  if (ERROR((rc = LIST_PUSH_BACK(&inf->outputs, output))))
    goto done;

done:
  return rc;
}
