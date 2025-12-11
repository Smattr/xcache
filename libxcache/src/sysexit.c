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

int sysexit(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  const unsigned long syscall_no = peek_syscall_no(thread);

  if (thread->ignoring) {
    DEBUG("TID %ld, ignoring sysexit %s«%lu» on spy’s instruction",
          (long)thread->id, syscall_to_str(syscall_no), syscall_no);

    goto done;
  }

// skip ignored syscalls and run them to their next event
#define SYSENTER_IGNORE(call) // nothing
#define SYSEXIT_IGNORE(call)                                                   \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("TID %ld, ignoring sysexit %s«%lu»", (long)thread->id, #call,      \
            syscall_no);                                                       \
      goto done;                                                               \
    }                                                                          \
  } while (0);
#include "ignore.h"

#define DO(call)                                                               \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      if (ERROR((rc = sysexit_##call(inf, thread)))) {                         \
        goto done;                                                             \
      }                                                                        \
                                                                               \
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
#ifdef __NR_readlinkat
  DO(readlinkat);
#endif
#ifdef __NR_clone
  DO(clone);
#endif
#ifdef __NR_fork
  DO(fork);
#endif
#ifdef __NR_vfork
  DO(vfork);
#endif
#ifdef __NR_clone3
  DO(clone3);
#endif
#ifdef __NR_readlink
  DO(readlink);
#endif
#ifdef __NR_chmod
  DO(chmod);
#endif
#ifdef __NR_getrandom
  DO(getrandom);
#endif
#ifdef __NR_pidfd_open
  DO(pidfd_open);
#endif
#ifdef __NR_faccessat2
  DO(faccessat2);
#endif

#undef DO

  DEBUG("TID %ld, unhandled sysexit %s«%lu»", (long)thread->id,
        syscall_to_str(syscall_no), syscall_no);
  rc = ENOTSUP;
done: {
  // restart the process
  int r = inferior_thread_continue(inf, thread, 0);
  if (ERROR(r != 0)) {
    if (rc == 0)
      rc = r;
  }
}

  return rc;
}
