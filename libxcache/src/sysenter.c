#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <errno.h>
#include <sys/syscall.h>

int sysenter(proc_t *proc) {

  int rc = 0;

  const long syscall_no = peek_syscall_no(proc->pid);
  DEBUG("sysenter %ld", syscall_no);

  // the vast majority of syscalls either (1) have no relevance to us or (2) we
  // prefer to handle at exit because the return value is available

// skip ignored syscalls and run them to their sysexit
#define IGNORE(call)                                                           \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("ignoring %s", #call);                                             \
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
