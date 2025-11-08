#include "debug.h"
#include "inferior_t.h"
#include "list.h"
#include "output_t.h"
#include <assert.h>
#include <stddef.h>

int inferior_output_new(inferior_t *inf, const output_t output) {

  assert(inf != NULL);

  int rc = 0;

  if (ERROR((rc = LIST_PUSH_BACK(&inf->outputs, output))))
    goto done;

done:
  return rc;
}
