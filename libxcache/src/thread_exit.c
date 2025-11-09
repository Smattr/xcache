#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

void thread_exit(thread_t *thread, int exit_status) {

  assert(thread != NULL);

  proc_free(thread->proc);
  thread->proc = NULL;

  thread->exit_status = exit_status;

  thread->id = 0;
}
