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

#define DO(call)                                                               \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      if (ERROR((rc = sysexit_##call(proc)))) {                                \
        goto done;                                                             \
      }                                                                        \
      goto done;                                                               \
    }                                                                          \
  } while (0)

#ifdef __NR_access
  DO(access);
#endif
#ifdef __NR_chdir
  DO(chdir);
#endif
#ifdef __NR_openat
  DO(openat);
#endif
#ifdef __NR_newfstatat
  DO(newfstatat);
#endif

#undef DO

  rc = ENOTSUP;
done:
  return rc;
}
