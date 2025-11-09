#include "debug.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <sys/ptrace.h>

int thread_syscall(thread_t thread) {

  assert(thread.id > 0);

  if (ERROR(ptrace(PTRACE_SYSCALL, thread.id, NULL, NULL) < 0)) {
    // if the thread disappeared when we were trying to resume it (can happen),
    // treat this as unsupported
    if (errno == ESRCH)
      return ECHILD;
    return errno;
  }

  return 0;
}
