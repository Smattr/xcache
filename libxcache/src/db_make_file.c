#include "db.h"
#include "debug.h"
#include "path.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int db_make_file(xc_db_t *db, FILE **fp, char **path) {

  assert(db != NULL);
  assert(fp != NULL);
  assert(path != NULL);

  int rc = -1;
  int fd = -1;
  FILE *f = NULL;

  // construct a template file path pointing into the database root
  char *template = path_join(db->root, "data.XXXXXX");
  if (ERROR(template == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // create a file at a new unique path
  fd = mkstemp(template);
  if (ERROR(fd == -1)) {
    rc = errno;
    goto done;
  }

  // turn the descriptor into a file handle
  f = fdopen(fd, "w");
  if (ERROR(f == NULL)) {
    rc = errno;
    goto done;
  }
  fd = -1;

  // success
  *fp = f;
  f = NULL;
  *path = template;
  template = NULL;
  rc = 0;

done:
  if (f != NULL)
    (void)fclose(f);
  if (fd >= 0) {
    (void)close(fd);
    assert(template != NULL);
    (void)unlink(template);
  }
  free(template);

  return rc;
}
