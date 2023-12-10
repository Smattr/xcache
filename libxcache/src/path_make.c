#include "debug.h"
#include "path.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int path_make(const char *root, const char *suffix, int *fd, char **path) {

  assert(root != NULL);
  assert(fd != NULL);

  char *stem = NULL;
  char *template = NULL;
  int rc = 0;

  if (ERROR(asprintf(&stem, "XXXXXX%s", suffix == NULL ? "" : suffix) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  template = path_join(root, stem);
  if (ERROR(template == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (suffix == NULL) {
    *fd = mkostemp(template, O_CLOEXEC);
  } else {
    *fd = mkostemps(template, strlen(suffix), O_CLOEXEC);
  }
  if (ERROR(*fd < 0)) {
    rc = errno;
    goto done;
  }

  if (path != NULL) {
    *path = template;
    template = NULL;
  }

done:
  free(template);
  free(stem);

  return rc;
}
