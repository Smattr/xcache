#include "channel.h"
#include "macros.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <xcache/proc.h>

int tracee_init(tracee_t *tracee, const xc_proc_t *proc) {

  assert(tracee != NULL);
  assert(proc != NULL);

  int rc = 0;
  memset(tracee, 0, sizeof(*tracee));

  // setup pipes for stdout, stderr
  if (UNLIKELY(pipe(tracee->out) != 0) || UNLIKELY(pipe(tracee->err) != 0)) {
    rc = errno;
    goto done;
  }

  // set close-on-exec on the read ends of the pipes, so the tracee does not
  // need to worry about closing them
  {
    int flags = fcntl(tracee->out[0], F_GETFD, 0);
    flags |= O_CLOEXEC;
    rc = fcntl(tracee->out[0], F_SETFD, flags);
    if (UNLIKELY(rc != 0))
      goto done;
  }
  {
    int flags = fcntl(tracee->err[0], F_GETFD, 0);
    flags |= O_CLOEXEC;
    rc = fcntl(tracee->err[0], F_SETFD, flags);
    if (UNLIKELY(rc != 0))
      goto done;
  }

  // setup a channel for the child to signal exec failure to the parent
  rc = channel_open(&tracee->msg);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  if (UNLIKELY(rc != 0))
    tracee_deinit(tracee);

  return rc;
}