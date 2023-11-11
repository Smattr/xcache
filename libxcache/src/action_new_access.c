#include "action_t.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

action_t *action_new_access(const char *path, int err, int flags) {

  assert(path != NULL);

  action_t *a = NULL;
  action_t *ret = NULL;

  a = calloc(1, sizeof(*a));
  if (ERROR(a == NULL))
    goto done;

  a->tag = ACT_ACCESS;
  a->err = err;
  a->access.flags = flags;

  ret = a;
  a = NULL;

done:
  action_free(a);

  return ret;
}
