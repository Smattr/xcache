#include "action_t.h"
#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

int action_new_read(action_t *action, int expected_err, const char *path) {

  assert(action != NULL);
  assert(path != NULL);

  *action = (action_t){0};
  action_t a = {0};
  int rc = 0;

  a.tag = ACT_READ;

  a.path = strdup(path);
  if (ERROR(a.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // TODO: handle EISDIR, mmap failure
  a.err = hash_file(path, &a.read.hash);

  // if we saw a different error to the child, assume it did something
  // unsupported
  if (ERROR(a.err != expected_err)) {
    rc = ECHILD;
    goto done;
  }

  *action = a;
  a = (action_t){0};

done:
  action_free(a);

  return rc;
}
