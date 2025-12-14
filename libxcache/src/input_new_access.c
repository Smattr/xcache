#include "debug.h"
#include "input.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

int input_new_access(input_t *input, const int *expected_err, const char *path,
                     int mode, int flags) {

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
    const int r = flags == 0 ? access(path, mode)
                             : faccessat(AT_FDCWD, path, mode, flags);
    if (ERROR(r < 0 && expected_err != NULL && errno != *expected_err)) {
      rc = ECHILD;
      goto done;
    }
    if (ERROR(r == 0 && expected_err != NULL && *expected_err != 0)) {
      rc = ECHILD;
      goto done;
    }
    i.err = r == 0 ? 0 : errno;
  }

  i.access.mode = mode;
  i.access.flags = flags;

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
