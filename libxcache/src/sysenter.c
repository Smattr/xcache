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
  DEBUG("sysenter %s (%lu)", syscall_to_str(syscall_no), syscall_no);

  // the vast majority of syscalls either (1) have no relevance to us or (2) we
  // prefer to handle at exit because the return value is available

// skip ignored syscalls and run them to their sysexit
#define IGNORE(call)                                                           \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("ignoring %s (%lu)", #call, syscall_no);                           \
      if (ERROR((rc = proc_syscall(*proc)))) {                                 \
        goto done;                                                             \
      }                                                                        \
      return 0;                                                                \
    }                                                                          \
  } while (0);
#include "ignore.h"

  // `execve` is one of the few syscalls we must handle on enter because the
  // caller’s address space does not exist at exit, making it impossible for us
  // to peek its arguments
#ifdef __NR_execve
  if (syscall_no == __NR_execve) {
    if (ERROR((rc = sysenter_execve(proc))))
      goto done;
    goto done;
  }
#endif

  rc = ENOTSUP;
done:
  return rc;
}
