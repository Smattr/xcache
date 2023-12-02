#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/syscall.h>

int sysenter(proc_t *proc) {

  assert(proc != NULL);

  int rc = 0;

  const unsigned long syscall_no = peek_syscall_no(proc->pid);
  DEBUG("pid %ld, sysenter %s«%lu»", (long)proc->pid,
        syscall_to_str(syscall_no), syscall_no);

  // the vast majority of syscalls either (1) have no relevance to us or (2) we
  // prefer to handle at exit because the return value is available

#define DO(call)                                                               \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      if (ERROR((rc = sysenter_##call(proc)))) {                               \
        goto done;                                                             \
      }                                                                        \
      goto done;                                                               \
    }                                                                          \
  } while (0)

// skip ignored syscalls and run them to their sysexit
#define SYSENTER_IGNORE(call)                                                  \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("ignoring %s«%lu»", #call, syscall_no);                            \
      if (ERROR((rc = proc_syscall(*proc)))) {                                 \
        goto done;                                                             \
      }                                                                        \
      goto done;                                                               \
    }                                                                          \
  } while (0);
#define SYSEXIT_IGNORE(call) // nothing
#include "ignore.h"

  // `execve` is one of the few syscalls we must handle on enter because the
  // caller’s address space does not exist at exit, making it impossible for us
  // to peek its arguments
  DO(execve);

  rc = ENOTSUP;
done:
  return rc;
}
