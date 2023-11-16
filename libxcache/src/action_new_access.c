#include "action_t.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

int action_new_access(action_t *action, int expected_err, const char *path,
                      int flags) {

  assert(action != NULL);
  assert(path != NULL);

  *action = (action_t){0};
  action_t a = {0};
  int rc = 0;

  a.tag = ACT_ACCESS;

  a.path = strdup(path);
  if (ERROR(a.path == NULL)) {
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

  a.err = expected_err;
  a.access.flags = flags;

  *action = a;
  a = (action_t){0};

done:
  action_free(a);

  return rc;
}
