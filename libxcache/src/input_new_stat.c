#include "debug.h"
#include "input.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

int input_new_stat(input_t *input, const int *expected_err, const char *path,
                   bool is_lstat) {

  assert(input != NULL);
  assert(path != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  i.tag = INP_STAT;

  i.path = strdup(path);
  if (ERROR(i.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  i.stat.is_lstat = is_lstat;
  if (expected_err != NULL && *expected_err != 0) {
    i.err = *expected_err;
  } else {
    struct stat st;
    const int r = is_lstat ? lstat(path, &st) : stat(path, &st);
    if (ERROR(expected_err != NULL && r < 0 && errno != *expected_err)) {
      rc = ECHILD;
      goto done;
    }
    if (ERROR(expected_err != NULL && r == 0 && *expected_err != 0)) {
      rc = ECHILD;
      goto done;
    }
    i.err = r == 0 ? 0 : errno;

    if (r == 0) {
      i.stat.mode = st.st_mode;
      i.stat.uid = st.st_uid;
      i.stat.gid = st.st_gid;
      i.stat.size = (size_t)st.st_size;
      i.stat.mtim = st.st_mtim;
      i.stat.ctim = st.st_ctim;
    }
  }

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
