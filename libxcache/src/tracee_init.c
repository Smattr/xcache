#include "channel.h"
#include "macros.h"
#include "tracee.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <xcache/proc.h>

static int set_cloexec(int fd) {
  int flags = fcntl(fd, F_GETFD, 0);
  flags |= O_CLOEXEC;
  if (UNLIKELY(fcntl(fd, F_SETFD, flags) != 0))
    return errno;
  return 0;
}

int tracee_init(tracee_t *tracee, const xc_proc_t *proc) {

  assert(tracee != NULL);
  assert(proc != NULL);

  int rc = 0;
  tracee_t t = {0};

  t.proc = proc;

  // setup pipes for stdout, stderr
  if (UNLIKELY(pipe(t.out) != 0) || UNLIKELY(pipe(t.err) != 0)) {
    rc = errno;
    goto done;
  }

  // set close-on-exec on the read ends of the pipes, so the tracee does not
  // need to worry about closing them
  rc = set_cloexec(t.out[0]);
  if (UNLIKELY(rc != 0))
    goto done;
  rc = set_cloexec(t.err[0]);
  if (UNLIKELY(rc != 0))
    goto done;

  // set the read ends of pipes non-blocking
  rc = set_nonblock(t.out[0]);
  if (UNLIKELY(rc != 0))
    goto done;
  rc = set_nonblock(t.err[0]);
  if (UNLIKELY(rc != 0))
    goto done;

  // setup a buffer for stdout
  t.out_content = open_memstream(&t.out_base, &t.out_len);
  if (UNLIKELY(t.out_content == NULL)) {
    rc = errno;
    goto done;
  }

  // setup a buffer for stderr
  t.err_content = open_memstream(&t.err_base, &t.err_len);
  if (UNLIKELY(t.err_content == NULL)) {
    rc = errno;
    goto done;
  }

  // setup a channel for the child to signal exec failure to the parent
  rc = channel_open(&t.msg);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  if (UNLIKELY(rc != 0)) {
    tracee_deinit(&t);
  } else {
    *tracee = t;
  }

  return rc;
}
