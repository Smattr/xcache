#include "debug.h"
#include "inferior_t.h"
#include "list.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

int sysexit_pidfd_open(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // extract PID
  const pid_t pid = (pid_t)peek_syscall_arg(thread, 1);

  DEBUG("TID %ld, pidfd_open(%ld, …)", (long)thread->id, (long)pid);

  // is the target PID a member of the tracee?
  bool is_internal = false;
  for (size_t i = 0; i < LIST_SIZE(&inf->threads); ++i) {
    const thread_t *const t = *LIST_AT(&inf->threads, i);
    if (t->proc->id != pid)
      continue;
    is_internal = true;
    break;
  }

  // give up if the tracee is trying to spy on something external
  if (ERROR(!is_internal)) {
    rc = ECHILD;
    goto done;
  }

  // otherwise we can ignore this

done:
  return rc;
}
