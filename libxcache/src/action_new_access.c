#include "action_t.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int action_new_access(action_t **action, const char *path, int err, int flags) {

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

  a->err = err;
  a->access.flags = flags;

  *action = a;
  a = NULL;

done:
  action_free(a);

  return rc;
}
