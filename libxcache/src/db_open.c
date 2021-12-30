#include "db.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <xcache/db.h>

/// make a canonical path, lacking redundant trailing slashes
static char *make_canon(const char *path) {

  assert(path != NULL);

  size_t length = strlen(path);
  while (strncmp(path, "/", length) != 0 && path[length - 1] == '/')
    --length;

  return strndup(path, length);
}

int xc_db_open(xc_db_t **db, const char *path) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  int rc = -1;

  xc_db_t *d = calloc(1, sizeof(*d));
  if (ERROR(d == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // save the root for later file construction
  d->root = make_canon(path);
  if (ERROR(d->root == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // construct the directory if it does not already exist
  if (mkdir(d->root, 0755) < 0) {
    if (ERROR(errno != EEXIST)) {
      rc = errno;
      goto done;
    }
  }

  // success
  *db = d;
  d = NULL;
  rc = 0;

done:
  xc_db_close(d);

  return rc;
}
