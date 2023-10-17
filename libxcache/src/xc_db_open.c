#include "db_t.h"
#include "debug.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xcache/db.h>

int xc_db_open(const char *path, xc_db_t **db) {

  if (ERROR(path == NULL))
    return EINVAL;

  if (ERROR(db == NULL))
    return EINVAL;

  xc_db_t *d = NULL;
  bool created = false;
  int rc = 0;

  d = calloc(1, sizeof(*d));
  if (ERROR(d == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (mkdir(path, 0755) < 0) {
    if (ERROR(errno != EEXIST)) {
      rc = errno;
      goto done;
    }
  } else {
    created = true;
  }

  // open the directory, now that it exists
  d->root = open(path, O_RDWR | O_CLOEXEC | O_DIRECTORY);
  if (ERROR(d->root < 0)) {
    rc = errno;
    goto done;
  }

  *db = d;
  d = NULL;

done:
  xc_db_close(d);
  if (rc && created)
    (void)rmdir(path);

  return rc;
}
