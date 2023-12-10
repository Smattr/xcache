#include "../../common/proccall.h"
#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <xcache/record.h>

int sysenter_ioctl(proc_t *proc) {

  assert(proc != NULL);

  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_reg(proc->pid, REG(rdi));

  // any ioctl except a communication from the spy is unsupported
  if (ERROR(fd != XCACHE_FILENO)) {
    rc = ECHILD;
    goto done;
  }

  // extract the call number
  const long callno = peek_reg(proc->pid, REG(rsi));

  DEBUG("pid %ld, ioctl(%d (XCACHE_FILENO), 0x%lx (%s), …)", (long)proc->pid,
        fd, callno, callno_to_str(callno));

  // dispatch call
  switch (callno) {
  case CALL_OFF:
    assert(!proc->ignoring && "duplicate monitor disable messages");
    proc->ignoring = true;
    break;

  case CALL_ON:
    assert(proc->ignoring && "duplicate monitor enable messages");
    proc->ignoring = false;
    break;

  default:
    DEBUG("unrecognised message from libxcache-spy: %ld", callno);
    assert(callno == CALL_OFF || callno == CALL_ON);
  }

  // restart the process
  if (proc->mode == XC_SYSCALL) {
    if (ERROR((rc = proc_syscall(*proc))))
      goto done;
  } else {
    if (ERROR((rc = proc_cont(*proc))))
      goto done;
  }

done:
  return rc;
}
