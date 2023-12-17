#include "../../common/proccall.h"
#include "debug.h"
#include "inferior_t.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <xcache/record.h>

int sysenter_ioctl(inferior_t *inf, proc_t *proc, thread_t *thread) {

  assert(inf != NULL);
  assert(proc != NULL);
  assert(thread != NULL);

  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_reg(thread, REG(rdi));

  // any ioctl except a communication from the spy is unsupported
  if (ERROR(fd != XCACHE_FILENO)) {
    DEBUG("TID %ld, ioctl(%d, …)", (long)thread->id, fd);
    if (thread->ignoring) {
      DEBUG("ignoring ioctl«%lu» on spy’s instruction",
            (unsigned long)__NR_ioctl);
      goto done;
    }
    rc = ECHILD;
    goto done;
  }

  // extract the call number
  const long callno = peek_reg(thread, REG(rsi));

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
