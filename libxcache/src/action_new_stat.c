#include "action_t.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

int action_new_stat(action_t *action, int expected_err, const char *path,
                    bool is_lstat) {

  assert(action != NULL);
  assert(path != NULL);

  *action = (action_t){0};
  action_t a = {0};
  int rc = 0;

  a.tag = ACT_STAT;

  a.path = strdup(path);
  if (ERROR(a.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  a.stat.is_lstat = is_lstat;
  {
    struct stat st;
    const int r = is_lstat ? lstat(path, &st) : stat(path, &st);
    if (ERROR(r < 0 && errno != expected_err)) {
      rc = ECHILD;
      goto done;
    }
    if (ERROR(r == 0 && expected_err != 0)) {
      rc = ECHILD;
      goto done;
    }
    a.err = expected_err;

    if (r == 0) {
      a.stat.mode = st.st_mode;
      a.stat.uid = st.st_uid;
      a.stat.gid = st.st_gid;
      a.stat.size = (size_t)st.st_size;
      a.stat.mtim = st.st_mtim;
      a.stat.ctim = st.st_ctim;
    }
  }

  *action = a;
  a = (action_t){0};

done:
  action_free(a);

  return rc;
}
