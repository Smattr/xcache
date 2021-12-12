#include "debug.h"
#include "macros.h"
#include "peek.h"
#include "tracee.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/syscall.h>

int syscall_middle(tracee_t *tracee) {

  assert(tracee != NULL);

  int rc = 0;

  // retrieve the syscall the child is midway through
  long nr = peek_syscall_no(tracee->pid);

  switch (nr) {

  // We need to handle `execve` here rather than at syscall exit because at
  // syscall exit the callee’s address space is gone, making it impossible to
  // read the (string) syscall argument. Conveniently, our handling of `execve`
  // does not depend on the syscall result.
  case __NR_execve: {

    // retrieve the path
    char *path = NULL;
    rc = peek_string(&path, tracee->pid, REG(rdi));
    if (UNLIKELY(rc != 0))
      goto done;

    rc = see_execve(tracee, path);
    free(path);
    if (UNLIKELY(rc != 0))
      goto done;

    rc = tracee_resume(tracee);
    if (UNLIKELY(rc != 0))
      goto done;

    break;
  }

  // any other syscalls we either do not care about or handle at syscall exit
  default:
    DEBUG("ignored seccomp stop for syscall %ld", nr);
    rc = tracee_resume_to_syscall(tracee);
    if (UNLIKELY(rc != 0))
      goto done;

    break;
  }

done:
  return rc;
}
