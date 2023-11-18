#include "debug.h"
#include "input_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

int input_new_access(input_t *input, int expected_err, const char *path,
                     int flags) {

  assert(input != NULL);
  assert(path != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  i.tag = INP_ACCESS;

  i.path = strdup(path);
  if (ERROR(i.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  {
    const int r = access(path, flags);
    if (ERROR(r < 0 && errno != expected_err)) {
      rc = ECHILD;
      goto done;
    }
    if (ERROR(r == 0 && expected_err != 0)) {
      rc = ECHILD;
      goto done;
    }
  }

  i.err = expected_err;
  i.access.flags = flags;

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
