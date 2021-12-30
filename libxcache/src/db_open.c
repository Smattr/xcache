#include "db.h"
#include "debug.h"
#include "macros.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/db.h>

int xc_db_open(xc_db_t **db, const char *path) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  int rc = 0;

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

done:
  if (UNLIKELY(rc != 0)) {
    xc_db_close(d);
  } else {
    *db = d;
  }

  return rc;
}
