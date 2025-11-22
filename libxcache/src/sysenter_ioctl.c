#include "../../common/proccall.h"
#include "debug.h"
#include "inferior_t.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <xcache/record.h>

int sysenter_ioctl(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_syscall_arg(thread, 1);

  // extract the call number
  const long callno = peek_syscall_arg(thread, 2);

  // ioctls we can reasonably ignore
  const struct {
    const char *name;
    long value;
  } SAFE[] = {
#define X(v) {#v, v}
      X(TCGETS),
#undef X
  };
  for (size_t i = 0; i < sizeof(SAFE) / sizeof(SAFE[i]); ++i) {
    if (callno == SAFE[i].value) {
      DEBUG("TID %ld, ignoring safe ioctl(%d, %s, …)", (long)thread->id, fd,
            SAFE[i].name);
      goto done;
    }
  }

  // any ioctl except a communication from the spy is unsupported
  if (ERROR(fd != XCACHE_FILENO)) {
    DEBUG("TID %ld, ioctl(%d, %ld, …)", (long)thread->id, fd, callno);
    if (thread->ignoring) {
      DEBUG("ignoring ioctl«%lu» on spy’s instruction",
            (unsigned long)__NR_ioctl);
      goto done;
    }
    rc = ECHILD;
    goto done;
  }

  DEBUG("TID %ld, ioctl(%d (XCACHE_FILENO), 0x%lx (%s), …)", (long)thread->id,
        fd, callno, callno_to_str(callno));

  // dispatch call
  switch (callno) {
  case CALL_OFF:
    assert(!thread->ignoring && "duplicate monitor disable messages");
    thread->ignoring = true;
    break;

  case CALL_ON:
    assert(thread->ignoring && "duplicate monitor enable messages");
    thread->ignoring = false;
    break;

  default:
    DEBUG("unrecognised message from libxcache-spy: %ld", callno);
    assert(callno == CALL_OFF || callno == CALL_ON);
  }

done:
  return rc;
}
