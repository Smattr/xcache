#include "debug.h"
#include "tracee.h"
#include <assert.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>

static long peek_reg(pid_t pid, size_t offset) {
  return ptrace(PTRACE_PEEKUSER, pid, offset, NULL);
}

#define PEEK(pid, reg)                                                         \
  peek_reg((pid), offsetof(struct user, regs) +                                \
                      offsetof(struct user_regs_struct, reg))

int witness_syscall(tracee_t *tracee) {

  assert(tracee != NULL);

  // retrieve the syscall number
  long nr = PEEK(tracee->pid, orig_rax);
  DEBUG("saw syscall %ld from the child", nr);

  // retrieve the result of the syscall
  long ret = PEEK(tracee->pid, rax);

  // TODO
  (void)nr;
  (void)ret;

  return 0;
}
