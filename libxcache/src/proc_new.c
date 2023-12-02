#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <xcache/record.h>

static int set_cloexec(int fd) {
  int flags = fcntl(fd, F_GETFD, 0);
  flags |= O_CLOEXEC;
  if (ERROR(fcntl(fd, F_SETFD, flags) < 0))
    return errno;
  return 0;
}

static int set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (ERROR(fcntl(fd, F_SETFL, flags) < 0))
    return errno;
  return 0;
}

int proc_new(proc_t *proc, unsigned mode) {

  assert(proc != NULL);

  *proc = (proc_t){0};
  proc_t p = {0};
  int rc = 0;

  // pick the newest mode available to us
  assert((mode & (XC_SYSCALL | XC_EARLY_SECCOMP | XC_LATE_SECCOMP)) != 0);
  if (mode & XC_LATE_SECCOMP) {
    p.mode = XC_LATE_SECCOMP;
  } else if (mode & XC_EARLY_SECCOMP) {
    p.mode = XC_EARLY_SECCOMP;
  } else {
    p.mode = XC_SYSCALL;
  }

  // setup a pipe for stdout
  if (ERROR(pipe(p.outfd) < 0)) {
    rc = errno;
    goto done;
  }

  // setup a pipe for stderr
  if (ERROR(pipe(p.errfd) < 0)) {
    rc = errno;
    goto done;
  }

  // set close-on-exec on the read ends of the pipes, so the tracee does not
  // need to worry about closing them
  if (ERROR((rc = set_cloexec(p.outfd[0]))))
    goto done;
  if (ERROR((rc = set_cloexec(p.errfd[0]))))
    goto done;

  // set the read ends of the pipes non-blocking
  if (ERROR((rc = set_nonblock(p.outfd[0]))))
    goto done;
  if (ERROR((rc = set_nonblock(p.errfd[0]))))
    goto done;

  // setup a bridge the subprocess will use to send us out-of-band messages
  if (ERROR(pipe(p.proccall) < 0)) {
    rc = errno;
    goto done;
  }

  *proc = p;
  p = (proc_t){0};

done:
  proc_free(p);

  return rc;
}
