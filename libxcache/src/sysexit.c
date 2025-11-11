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
  DEBUG("TID %ld, sysexit %s«%lu»", (long)thread->id,
        syscall_to_str(syscall_no), syscall_no);

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

// skip ignored syscalls and run them to their next event
#define SYSENTER_IGNORE(call) // nothing
#define SYSEXIT_IGNORE(call)                                                   \
  do {                                                                         \
    if (syscall_no == __NR_##call) {                                           \
      DEBUG("ignoring %s«%lu»", #call, syscall_no);                            \
      if (inf->mode == XC_SYSCALL) {                                           \
        if (ERROR((rc = thread_syscall(*thread)))) {                           \
          goto done;                                                           \
        }                                                                      \
      } else {                                                                 \
        if (ERROR((rc = thread_cont(*thread)))) {                              \
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
      if (ERROR((rc = sysexit_##call(inf, thread)))) {                         \
        goto done;                                                             \
      }                                                                        \
                                                                               \
      /* restart the process */                                                \
      if (inf->mode == XC_SYSCALL) {                                           \
        if (ERROR((rc = thread_syscall(*thread))))                             \
          goto done;                                                           \
      } else {                                                                 \
        if (ERROR((rc = thread_cont(*thread))))                                \
          goto done;                                                           \
      }                                                                        \
                                                                               \
      goto done;                                                               \
    }                                                                          \
  } while (0)

#ifdef __NR_close
  DO(close);
#endif
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

#undef DO

  rc = ENOTSUP;
done:
  return rc;
}
