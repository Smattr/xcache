#include "debug.h"
#include "hash_t.h"
#include "input_t.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

/// which errno values should be anticipated as informational, not errors
static bool accepted_errno(int err) {
  if (err == 0)
    return true;
  if (err == ENOENT)
    return true;
  if (err == EACCES || err == EPERM)
    return true;
  if (err == EISDIR)
    return true;
  return false;
}

int input_new(input_t *input, const char *path) {

  assert(input != NULL);
  assert(path != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  // save the path
  i.path = strdup(path);
  if (ERROR(i.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // read the target’s attributes
  {
    struct stat st = {0};
    if (stat(path, &st) < 0) {
      if (ERROR(!accepted_errno(errno))) {
        rc = errno;
        goto done;
      }
      i.stat_errno = errno;
    } else {
      // only accept directories and regular files
      if (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)) {
        rc = ENOTSUP;
        goto done;
      }

      i.stat_errno = 0;
      i.st_mode = st.st_mode;
      i.st_uid = st.st_uid;
      i.st_gid = st.st_gid;
      i.st_size = (size_t)st.st_size;
    }
  }

  // hash it
  if (i.stat_errno == 0 && S_ISREG(i.st_mode) && i.st_size > 0) {
    i.open_errno = hash_file(path, &i.digest);
    if (ERROR(!accepted_errno(i.open_errno))) {
      rc = i.open_errno;
      goto done;
    }
  }

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
