#include "debug.h"
#include "inferior_t.h"
#include "io.h"
#include "list.h"
#include "output.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

int inferior_output_new(inferior_t *inf, const output_t output) {

  assert(inf != NULL);

  int rc = 0;

  // we can drop any earlier output that is dominated by this output
  const io_t io = {.tag = IO_OUTPUT, .output = output};
  for (size_t i = LIST_SIZE(&inf->io) - 1; i != SIZE_MAX; --i) {
    io_t prior = *LIST_AT(&inf->io, i);
    const dep_t dep = io_cmp(prior, io);
    assert(dep != DEP_RAR);
    assert(dep != DEP_RAW);
    if (dep == DEP_WAW) {
      (void)LIST_POP(&inf->io, i);
      io_free(prior);
    } else if (dep == DEP_WAR || dep == DEP_CTRL) {
      break;
    }
  }

  if (ERROR((rc = LIST_PUSH_BACK(&inf->io, io))))
    goto done;

done:
  return rc;
}
