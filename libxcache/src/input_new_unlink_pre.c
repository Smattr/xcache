#include "debug.h"
#include "input.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

int input_new_unlink_pre(input_t *input, const char *path) {
  assert(input != NULL);
  assert(path != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  i.tag = INP_UNLINK_PRE;

  i.path = strdup(path);
  if (ERROR(i.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
