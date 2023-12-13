#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <sys/ptrace.h>

int proc_syscall(const proc_t proc) {

  assert(proc.pid > 0);

  if (ERROR(ptrace(PTRACE_SYSCALL, proc.pid, NULL, NULL) < 0)) {
    // if the child disappeared when we were trying to resume it (can happen),
    // treat this as unsupported
    if (errno == ESRCH)
      return ECHILD;
    return errno;
  }

  return 0;
}
