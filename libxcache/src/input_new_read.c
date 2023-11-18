#include "debug.h"
#include "hash_t.h"
#include "input_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

int input_new_read(input_t *input, int expected_err, const char *path) {

  assert(input != NULL);
  assert(path != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  i.tag = INP_READ;

  i.path = strdup(path);
  if (ERROR(i.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // TODO: handle EISDIR, mmap failure
  i.err = hash_file(path, &i.read.hash);

  // if we saw a different error to the child, assume it did something
  // unsupported
  if (ERROR(i.err != expected_err)) {
    rc = ECHILD;
    goto done;
  }

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
