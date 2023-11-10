#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <errno.h>
#include <sys/syscall.h>

int sysenter(proc_t *proc) {

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

  rc = ENOTSUP;
done:
  return rc;
}
