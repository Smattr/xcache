#include "debug.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int fd_new(fd_t **fd, const char *path) {

  assert(fd != NULL);
  assert(path != NULL);

  *fd = NULL;
  fd_t *f = NULL;
  int rc = 0;

  f = calloc(1, sizeof(*f));
  if (ERROR(f == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  f->path = strdup(path);
  if (ERROR(f->path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  *fd = f;
  f = NULL;

done:
  fd_free(f);

  return rc;
}
