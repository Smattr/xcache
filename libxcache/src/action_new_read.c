#include "action_t.h"
#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int action_new_read(action_t **action, int expected_err, const char *path) {

  assert(action != NULL);
  assert(path != NULL);

  *action = NULL;
  action_t *a = NULL;
  int rc = 0;

  a = calloc(1, sizeof(*a));
  if (ERROR(a == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  a->tag = ACT_READ;

  a->path = strdup(path);
  if (ERROR(a->path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // TODO: handle EISDIR, mmap failure
  a->err = hash_file(path, &a->read.hash);

  // if we saw a different error to the child, assume it did something
  // unsupported
  if (ERROR(a->err != expected_err)) {
    rc = ECHILD;
    goto done;
  }

  *action = a;
  a = NULL;

done:
  action_free(a);

  return rc;
}
