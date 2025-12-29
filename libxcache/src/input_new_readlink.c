#include "debug.h"
#include "hash_t.h"
#include "input.h"
#include "path.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int input_new_readlink(input_t *input, const int *expected_err,
                       const char *path) {

  assert(input != NULL);
  assert(path != NULL);

  *input = (input_t){0};
  input_t i = {0};
  char *target = NULL;
  int rc = 0;

  i.tag = INP_READLINK;

  i.path = strdup(path);
  if (ERROR(i.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (expected_err != NULL && *expected_err != 0) {
    i.err = *expected_err;
  } else {
    i.err = readln(path, &target);

    // if we saw a different error to the child, assume it did something
    // unsupported
    if (ERROR(expected_err != NULL && i.err != *expected_err)) {
      rc = ECHILD;
      goto done;
    }

    if (i.err == 0)
      i.readlink.hash = hash_data(target, strlen(target));
  }

  *input = i;
  i = (input_t){0};

done:
  free(target);
  input_free(i);

  return rc;
}
