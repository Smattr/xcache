#include "debug.h"
#include "macros.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/ptrace.h>

int tracee_resume_to_syscall(tracee_t *tracee) {

  assert(tracee != NULL);

  DEBUG("resuming %d...", (int)tracee->pid);
  if (UNLIKELY(ptrace(PTRACE_SYSCALL, tracee->pid, NULL, NULL) != 0)) {
    int rc = errno;
    DEBUG("failed to resume %d: %d", (int)tracee->pid, rc);
    return rc;
  }

  return 0;
}
