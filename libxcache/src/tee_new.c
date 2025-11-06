#include "debug.h"
#include "path.h"
#include "tee_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

/// entry point of a child IO thread
///
/// @param tee Thread self pointer
/// @return NULL on success or an errno on failure
static void *forward(void *tee) {

  assert(tee != NULL);

  const tee_t *t = tee;
  int rc = 0;

  // our parent should have set us up correctly
  assert(t->pipe[0] > 0);
  assert(t->output_fd >= 0);
  assert(t->copy_fd > 0);

  while (true) {

    // read a chunk from the source
    char buf[BUFSIZ];
    const ssize_t r = read(t->pipe[0], buf, sizeof(buf));
    if (ERROR(r < 0)) {
      if (errno == EINTR)
        continue;
      rc = errno;
      goto done;
    }
    assert((size_t)r <= sizeof(buf));
    if (r == 0) // EOF
      break;

    // write it to the main output
    for (size_t len = (size_t)r; len > 0;) {
      const ssize_t w = write(t->output_fd, buf, len);
      if (ERROR(w < 0)) {
        if (errno == EINTR)
          continue;
        rc = errno;
        goto done;
      }
      assert((size_t)w <= len);
      len -= (size_t)w;
    }

    // write it to the copy
    for (size_t len = (size_t)r; len > 0;) {
      const ssize_t w = write(t->copy_fd, buf, len);
      if (ERROR(w < 0)) {
        if (errno == EINTR)
          continue;
        rc = errno;
        goto done;
      }
      assert((size_t)w <= len);
      len -= (size_t)w;
    }
  }

done:
  return (void *)(intptr_t)rc;
}

int tee_new(tee_t **tee, int output_fd, const char *trace_root) {

  assert(tee != NULL);
  assert(output_fd >= 0);
  assert(trace_root != NULL);

  *tee = NULL;
  tee_t *t = NULL;
  int rc = 0;

  t = calloc(1, sizeof(*t));
  if (ERROR(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (ERROR(pipe2(t->pipe, O_CLOEXEC) < 0)) {
    rc = errno;
    goto done;
  }

  t->output_fd = output_fd;

  if (ERROR((rc = path_make(trace_root, NULL, &t->copy_fd, &t->copy_path))))
    goto done;

  DEBUG("piping file descriptor %d to %s", output_fd, t->copy_path);

  if (ERROR((rc = pthread_create(&t->forwarder, NULL, forward, t))))
    goto done;
  t->initialised = true;

  *tee = t;
  t = NULL;

done:
  tee_cancel(t);

  return rc;
}
