#include "db.h"
#include "macros.h"
#include "path.h"
#include <errno.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcache/db.h>

int xc_db_open(xc_db_t **db, const char *path, int flags) {

  if (db == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  if (flags != O_RDWR && flags != O_RDONLY && flags != (O_RDWR | O_CREAT))
    return EINVAL;

  int rc = 0;
  char *db_path = NULL;

  xc_db_t *d = calloc(1, sizeof(*d));
  if (UNLIKELY(d == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  d->read_only = !(flags & O_RDWR);

  // construct a path to the database
  db_path = path_join(path, "xcache.db");
  if (UNLIKELY(db_path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // does the database already exist?
  bool exists = true;
  if (access(db_path, F_OK) != 0) {
    if (errno == ENOENT) {
      exists = false;
    } else {
      rc = errno;
      goto done;
    }
  }

  // if it does not exist and we were not asked to create it, we are done
  if (!exists && !(flags & O_CREAT))
    goto done;

  // open the database itself
  {
    int mode = (flags & O_RDWR) ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
                                : SQLITE_OPEN_READONLY;
    int r = sqlite3_open_v2(db_path, &d->db, mode, NULL);
    if (UNLIKELY(r != SQLITE_OK)) {
      // TODO: translate Sqlite errors into errno
      rc = ENOTSUP;
      goto done;
    }
  }

done:
  if (UNLIKELY(rc != 0)) {
    xc_db_close(d);
  } else {
    *db = d;
  }
  free(db_path);

  return rc;
}
