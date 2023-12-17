#include "debug.h"
#include "inferior_t.h"
#include "tee_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcache/record.h>

int inferior_new(inferior_t *inf, unsigned mode, const char *trace_root) {

  assert(inf != NULL);
  assert(trace_root != NULL);

  *inf = (inferior_t){0};
  inferior_t i = {0};
  int rc = 0;

  // pick the newest mode available to us
  assert((mode & (XC_SYSCALL | XC_EARLY_SECCOMP | XC_LATE_SECCOMP)) != 0);
  if (mode & XC_LATE_SECCOMP) {
    i.mode = XC_LATE_SECCOMP;
  } else if (mode & XC_EARLY_SECCOMP) {
    i.mode = XC_EARLY_SECCOMP;
  } else {
    i.mode = XC_SYSCALL;
  }

  // setup a pipe for stdout
  if (ERROR((rc = tee_new(&i.t_out, STDOUT_FILENO, trace_root))))
    goto done;

  // setup a pipe for stderr
  if (ERROR((rc = tee_new(&i.t_err, STDERR_FILENO, trace_root))))
    goto done;

  // setup a bridge the subprocess will use to send us out-of-band messages
  if (ERROR(pipe2(i.proccall, O_CLOEXEC) < 0)) {
    rc = errno;
    goto done;
  }

  *inf = i;
  i = (inferior_t){0};

done:
  inferior_free(&i);

  return rc;
}
