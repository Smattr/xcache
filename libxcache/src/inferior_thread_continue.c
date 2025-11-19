#include "debug.h"
#include "inferior_t.h"
#include "thread_t.h"
#include <errno.h>
#include <sys/ptrace.h>

int inferior_thread_continue(const inferior_t *inf, const thread_t *subject,
                             int sig) {

  // how we resume depends on how we are tracing:
  //   • ptrace → need to stop at the next syscall
  //   • seccomp → free running; will stop at the next seccomp
  const int op = inf->mode == XC_SYSCALL ? PTRACE_SYSCALL : PTRACE_CONT;

  if (ERROR(ptrace(op, subject->id, NULL, sig) < 0)) {
    // if the thread disappeared when we were trying to signal it (can happen),
    // treat this as unsupported
    if (errno == ESRCH)
      return ECHILD;
    return errno;
  }

  return 0;
}
