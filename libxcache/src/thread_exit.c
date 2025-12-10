#include "fs.h"
#include "proc.h"
#include "thread_t.h"
#include <assert.h>
#include <stdlib.h>

void thread_exit(thread_t *thread, int exit_status) {

  assert(thread != NULL);

  thread->fs = fs_release(thread->fs);

  thread->proc = proc_release(thread->proc);

  if (thread->exit_status != NULL)
    *thread->exit_status = exit_status;

  free(thread);
}
