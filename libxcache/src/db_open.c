#include "db.h"
#include "debug.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <xcache/db.h>

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
  d->root = strdup(path);
  if (ERROR(d->root == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // construct the directory if it does not already exist
  if (mkdir(path, 0755) < 0) {
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
