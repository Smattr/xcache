#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/syscall.h>

int sysexit(proc_t *proc) {

  assert(proc != NULL);

  int rc = 0;

  const unsigned long syscall_no = peek_syscall_no(proc->pid);
  DEBUG("sysexit %s (%lu)", syscall_to_str(syscall_no), syscall_no);

#ifdef __NR_chdir
  if (syscall_no == __NR_chdir) {
    if (ERROR((rc = sysexit_chdir(proc))))
      goto done;
    goto done;
  }
#endif

  rc = ENOTSUP;
done:
  return rc;
}
