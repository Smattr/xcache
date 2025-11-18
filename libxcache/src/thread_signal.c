#include "debug.h"
#include "thread_t.h"
#include <errno.h>
#include <stdbool.h>
#include <sys/ptrace.h>

int thread_signal(thread_t thread, int sig, bool cont) {

  if (ERROR(ptrace(cont ? PTRACE_CONT : PTRACE_SYSCALL, thread.id, NULL, sig) <
            0)) {
    // if the thread disappeared when we were trying to signal it (can happen),
    // treat this as unsupported
    if (errno == ESRCH)
      return ECHILD;
    return errno;
  }

  return 0;
}
