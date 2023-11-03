#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <sys/ptrace.h>

int proc_signal(const proc_t proc, int sig) {

  assert(proc.pid > 0);

  if (ERROR(ptrace(PTRACE_CONT, proc.pid, NULL, sig) < 0))
    return errno;

  return 0;
}
