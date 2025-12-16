#include "debug.h"
#include "inferior_t.h"
#include "input.h"
#include "io.h"
#include "list.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

int inferior_input_new(inferior_t *inf, input_t input) {

  assert(inf != NULL);

  int rc = 0;

  // we can elide this input if it is dominated by an earlier action
  const io_t io = {.tag = IO_INPUT, .input = input};
  for (size_t i = LIST_SIZE(&inf->io) - 1; i != SIZE_MAX; --i) {
    const io_t prior = *LIST_AT(&inf->io, i);
    const dep_t dep = io_cmp(prior, io);
    assert(dep != DEP_WAR);
    assert(dep != DEP_WAW);
    if (dep == DEP_RAR || dep == DEP_RAW) {
      input_free(input);
      goto done;
    } else if (dep == DEP_CTRL) {
      break;
    }
  }

  if (ERROR((rc = LIST_PUSH_BACK(&inf->io, io))))
    goto done;

done:
  return rc;
}
