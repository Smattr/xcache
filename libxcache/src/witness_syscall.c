#include "debug.h"
#include "peek.h"
#include "tracee.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>

int witness_syscall(tracee_t *tracee) {

  assert(tracee != NULL);

  // retrieve the syscall number
  long nr = peek_syscall_no(tracee->pid);

  // retrieve the result of the syscall
  long ret = peek_reg(tracee->pid, REG(rax));

  switch (nr) {

#ifdef __NR_chdir
  case __NR_chdir: {

    // retrieve the path
    char *path = NULL;
    int rc = peek_string(&path, tracee->pid, REG(rdi));
    if (UNLIKELY(rc != 0))
      return rc;

    rc = witness_chdir(tracee, ret, path);

    free(path);
    return rc;
  }
#endif

  default:
    DEBUG("unrecognised syscall %ld", nr);
  }

  return 0;
}
