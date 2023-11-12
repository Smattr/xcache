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
  DEBUG("pid %ld, sysexit %s«%lu»", (long)proc->pid, syscall_to_str(syscall_no),
        syscall_no);

// skip ignored syscalls and run them to their next event
#define SYSENTER_IGNORE(call) // notthing
#define SYSEXIT_IGNORE(call)                                                   \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("ignoring %s«%lu»", #call, syscall_no);                            \
      if (proc->mode == XC_SYSCALL) {                                          \
        if (ERROR((rc = proc_syscall(*proc)))) {                               \
          goto done;                                                           \
        }                                                                      \
      } else {                                                                 \
        if (ERROR((rc = proc_cont(*proc)))) {                                  \
          goto done;                                                           \
        }                                                                      \
      }                                                                        \
      goto done;                                                               \
    }                                                                          \
  } while (0);
#include "ignore.h"

#ifdef __NR_access
  if (syscall_no == __NR_access) {
    if (ERROR((rc = sysexit_access(proc))))
      goto done;
    goto done;
  }
#endif
#ifdef __NR_chdir
  if (syscall_no == __NR_chdir) {
    if (ERROR((rc = sysexit_chdir(proc))))
      goto done;
    goto done;
  }
#endif
#ifdef __NR_openat
  if (syscall_no == __NR_openat) {
    if (ERROR((rc = sysexit_openat(proc))))
      goto done;
    goto done;
  }
#endif
#ifdef __NR_newfstatat
  if (syscall_no == __NR_newfstatat) {
    if (ERROR((rc = sysexit_newfstatat(proc))))
      goto done;
    goto done;
  }
#endif

  rc = ENOTSUP;
done:
  return rc;
}
