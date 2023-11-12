#include "proc_t.h"
#include <stdlib.h>

void fd_free(fd_t *fd) {

  if (fd == NULL)
    return;

  free(fd->path);
  free(fd);
}
