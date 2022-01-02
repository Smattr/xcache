#include "debug.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/ptrace.h>

int tracee_resume_to_exit(tracee_t *tracee) {

  assert(tracee != NULL);

  DEBUG("detaching %d...", (int)tracee->pid);
  if (ERROR(ptrace(PTRACE_DETACH, tracee->pid, NULL, NULL) != 0))
    return errno;

  return 0;
}
