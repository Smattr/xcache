#include "action_t.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int action_new_access(action_t **action, int expected_err, const char *path,
                      int flags) {

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

  a->tag = ACT_ACCESS;

  a->path = strdup(path);
  if (ERROR(a->path == NULL)) {
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

  a->err = expected_err;
  a->access.flags = flags;

  *action = a;
  a = NULL;

done:
  action_free(a);

  return rc;
}
