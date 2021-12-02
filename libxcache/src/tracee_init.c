#include "channel.h"
#include "db.h"
#include "macros.h"
#include "tracee.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xcache/db_t.h>
#include <xcache/proc.h>

static int set_cloexec(int fd) {
  int flags = fcntl(fd, F_GETFD, 0);
  flags |= O_CLOEXEC;
  if (UNLIKELY(fcntl(fd, F_SETFD, flags) != 0))
    return errno;
  return 0;
}

int tracee_init(tracee_t *tracee, const xc_proc_t *proc, xc_db_t *db) {

  assert(tracee != NULL);
  assert(proc != NULL);
  assert(db != NULL);

  int rc = 0;
  memset(tracee, 0, sizeof(*tracee));

  tracee->proc = proc;

  // setup pipes for stdout, stderr
  if (UNLIKELY(pipe(tracee->out) != 0) || UNLIKELY(pipe(tracee->err) != 0)) {
    rc = errno;
    goto done;
  }

  // set close-on-exec on the read ends of the pipes, so the tracee does not
  // need to worry about closing them
  rc = set_cloexec(tracee->out[0]);
  if (UNLIKELY(rc != 0))
    goto done;
  rc = set_cloexec(tracee->err[0]);
  if (UNLIKELY(rc != 0))
    goto done;

  // set the read ends of pipes non-blocking
  rc = set_nonblock(tracee->out[0]);
  if (UNLIKELY(rc != 0))
    goto done;
  rc = set_nonblock(tracee->err[0]);
  if (UNLIKELY(rc != 0))
    goto done;

  // setup a file to save stdout
  rc = db_make_file(db, &tracee->out_f, &tracee->out_path);
  if (UNLIKELY(rc != 0))
    goto done;

  // setup a file to save stderr
  rc = db_make_file(db, &tracee->err_f, &tracee->err_path);
  if (UNLIKELY(rc != 0))
    goto done;

  // setup a channel for the child to signal exec failure to the parent
  rc = channel_open(&tracee->msg);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  if (UNLIKELY(rc != 0))
    tracee_deinit(tracee);

  return rc;
}
