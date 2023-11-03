#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <sys/ptrace.h>

int proc_syscall(const proc_t proc) {

  assert(proc.pid > 0);

  if (ERROR(ptrace(PTRACE_SYSCALL, proc.pid, NULL, NULL) < 0))
    return errno;

  return 0;
}
