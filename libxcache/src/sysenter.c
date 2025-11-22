#include "debug.h"
#include "inferior_t.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <xcache/record.h>

int sysenter(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  const unsigned long syscall_no = peek_syscall_no(thread);

  // the vast majority of syscalls either (1) have no relevance to us or (2) we
  // prefer to handle at exit because the return value is available

#define DO(call)                                                               \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      if (ERROR((rc = sysenter_##call(inf, thread)))) {                        \
        goto done;                                                             \
      }                                                                        \
                                                                               \
      goto done;                                                               \
    }                                                                          \
  } while (0)

  // handle ioctl before checking whether we are ignoring, because the ioctl
  // might be an instruction to stop ignoring
  DO(ioctl);

  if (thread->ignoring) {
    DEBUG("TID %ld, ignoring sysenter %s«%lu» on spy’s instruction",
          (long)thread->id, syscall_to_str(syscall_no), syscall_no);

    goto done;
  }

// skip ignored syscalls and run them to their sysexit
#define SYSENTER_IGNORE(call)                                                  \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("TID %ld, ignoring sysenter %s«%lu»", (long)thread->id, #call,     \
            syscall_no);                                                       \
      goto done;                                                               \
    }                                                                          \
  } while (0);
#define SYSEXIT_IGNORE(call) // nothing
#include "ignore.h"

#ifdef __NR_clone
  DO(clone);
#endif
#ifdef __NR_clone3
  DO(clone3);
#endif
#ifdef __NR_fork
  DO(fork);
#endif
#ifdef __NR_vfork
  DO(vfork);
#endif

  // `execve` is one of the few syscalls we must handle on enter because the
  // caller’s address space does not exist at exit, making it impossible for us
  // to peek its arguments
#ifdef __NR_execve
  DO(execve);
#endif

  // We need to handle a subset of `openat` calls that create the target file
  // conditionally dependent on whether it already exists. We cannot do this on
  // sysexit because the file is already created by then.
#ifdef __NR_openat
  DO(openat);
#endif

  DEBUG("TID %ld, unhandled sysenter %s«%lu»", (long)thread->id,
        syscall_to_str(syscall_no), syscall_no);
  rc = ENOTSUP;
done: {
  int r = inferior_thread_continue(inf, thread, 0);
  /* restart the process */
  if (ERROR(r != 0)) {
    if (rc == 0)
      rc = r;
  }
}

  return rc;
}
