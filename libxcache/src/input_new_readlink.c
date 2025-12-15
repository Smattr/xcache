#include "debug.h"
#include "hash_t.h"
#include "input.h"
#include "path.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int input_new_readlink(input_t *input, int expected_err, const char *path) {

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

  i.err = readln(path, &target);

  // if we saw a different error to the child, assume it did something
  // unsupported
  if (ERROR(i.err != expected_err)) {
    rc = ECHILD;
    goto done;
  }

  i.readlink.hash = hash_data(target, strlen(target));

  *input = i;
  i = (input_t){0};

done:
  free(target);
  input_free(i);

  return rc;
}
