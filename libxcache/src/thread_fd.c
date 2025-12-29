#include "debug.h"
#include "path.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int thread_fd(thread_t *thread, int fd, char **path) {
  assert(thread != NULL);
  assert(path != NULL);

  *path = NULL;
  char *proc = NULL;
  char *target = NULL;
  int rc = 0;

  if (ERROR(fd < 0)) {
    rc = EINVAL;
    goto done;
  }

  // construct a path to the procfs symlink for this descriptor
  if (ERROR(asprintf(&proc, "/proc/%ld/task/%ld/fd/%d", (long)thread->proc->id,
                     (long)thread->id, fd) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  DEBUG("readlink \"%s\"…", proc);
  if (ERROR((rc = readln(proc, &target))))
    goto done;
  DEBUG("readlink \"%s\" = \"%s\"", proc, target);

  *path = target;
  target = NULL;

done:
  free(target);
  free(proc);

  return rc;
}
