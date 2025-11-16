#include "fd.h"
#include "fs.h"
#include "thread_t.h"
#include <assert.h>
#include <stdlib.h>

void thread_exit(thread_t *thread, int exit_status) {

  assert(thread != NULL);

  thread->fd = fds_release(thread->fd);

  thread->fs = fs_release(thread->fs);

  proc_free(thread->proc);
  thread->proc = NULL;

  if (thread->exit_status != NULL)
    *thread->exit_status = exit_status;

  free(thread);
}
