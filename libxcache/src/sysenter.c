#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <xcache/record.h>
#include "inferior_t.h"

int sysenter(inferior_t *inf, proc_t *proc, thread_t *thread) {

  assert(inf != NULL);
  assert(proc != NULL);
  assert(thread!= NULL);

  int rc = 0;

  const unsigned long syscall_no = peek_syscall_no(thread);
  DEBUG("TID %ld, sysenter %s«%lu»", (long)thread->id,
        syscall_to_str(syscall_no), syscall_no);

  // the vast majority of syscalls either (1) have no relevance to us or (2) we
  // prefer to handle at exit because the return value is available

#define DO(call)                                                               \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      if (ERROR((rc = sysenter_##call(inf, proc, thread)))) {                               \
        goto done;                                                             \
      }                                                                        \
\
    /* restart the process */ \
    if (inf->mode == XC_SYSCALL) {\
      if (ERROR((rc = thread_syscall(*thread))))\
        goto done;\
    } else {\
      if (ERROR((rc = thread_cont(*thread))))\
        goto done;\
    }\
\
goto done; \
    }                                                                          \
  } while (0)

  // handle ioctl before checking whether we are ignoring, because the ioctl
  // might be an instruction to stop ignoring
  DO(ioctl);

  if (thread->ignoring) {
    DEBUG("ignoring %s«%lu» on spy’s instruction", syscall_to_str(syscall_no),
          syscall_no);

    // restart the process
    if (inf->mode == XC_SYSCALL) {
      if (ERROR((rc = thread_syscall(*thread))))
        goto done;
    } else {
      if (ERROR((rc = thread_cont(*thread))))
        goto done;
    }

    goto done;
  }

// skip ignored syscalls and run them to their sysexit
#define SYSENTER_IGNORE(call)                                                  \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("ignoring %s«%lu»", #call, syscall_no);                            \
      if (ERROR((rc = thread_syscall(*thread)))) {                                 \
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
