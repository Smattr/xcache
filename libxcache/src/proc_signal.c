#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <sys/ptrace.h>

int proc_signal(const proc_t proc, int sig) {

  assert(proc.pid > 0);

  if (ERROR(ptrace(PTRACE_CONT, proc.pid, NULL, sig) < 0)) {
    // if the child disappeared when we were trying to signal it (can happen),
    // treat this as unsupported
    if (errno == ESRCH)
      return ECHILD;
    return errno;
  }

  return 0;
}
