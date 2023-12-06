#include "debug.h"
#include "proc_t.h"
#include "tee_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <xcache/record.h>

int proc_new(proc_t *proc, unsigned mode, const char *trace_root) {

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
  if (ERROR((rc = tee_new(&p.t_out, STDOUT_FILENO, trace_root))))
    goto done;

  // setup a pipe for stderr
  if (ERROR((rc = tee_new(&p.t_err, STDERR_FILENO, trace_root))))
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
