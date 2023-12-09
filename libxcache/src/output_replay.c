#include "cp.h"
#include "debug.h"
#include "output_t.h"
#include "trace_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xcache/trace.h>

static int replay_chmod(const output_t output) {

  assert(output.tag == OUT_CHMOD);

  int rc = 0;

  DEBUG("replaying chmod(\"%s\", %ld)", output.path, (long)output.chmod.mode);
  if (ERROR(chmod(output.path, output.chmod.mode) < 0)) {
    rc = errno;
    goto done;
  }

done:
  return rc;
}

static int replay_chown(const output_t output) {

  assert(output.tag == OUT_CHOWN);

  int rc = 0;

  DEBUG("replaying chown(\"%s\", %ld, %ld)", output.path, (long)output.chown.uid, (long)output.chown.gid);
  if (ERROR(chown(output.path, output.chown.uid, output.chown.gid) < 0)) {
    rc = errno;
    goto done;
  }

done:
  return rc;
}

static int replay_mkdir(const output_t output) {

  assert(output.tag == OUT_MKDIR);

  int rc = 0;

  DEBUG("replaying mkdir(\"%s\", %ld)", output.path, (long)output.mkdir.mode);
  if (ERROR(mkdir(output.path, output.mkdir.mode) < 0)) {
    // allow directory to already exist
    if (ERROR(errno != EEXIST)) {
      rc = errno;
      goto done;
    }

    // if the directory existed, try to enforce our mode
    if (ERROR(chmod(output.path, output.mkdir.mode) < 0)) {
      rc = errno;
      goto done;
    }
  }

done:
  return rc;
}

static int replay_write(const output_t output, const xc_trace_t *owner) {

  assert(output.tag == OUT_WRITE);

  if (ERROR(output.write.cached_copy == NULL))
    return EINVAL;

  int src = -1;
  int dst = -1;
  int rc = 0;

  DEBUG("retrieving cached content from %s", output.write.cached_copy);
  src = openat(owner->root, output.write.cached_copy, O_RDONLY | O_CLOEXEC);
  if (ERROR(src < 0)) {
    rc = errno;
    goto done;
  }

  // we do not care so much about the mode because anything that was opened
  // `O_CREAT` will have a following `chmod` action recorded too
  DEBUG("replaying open(\"%s\", …)", output.path);
  dst = open(output.path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (ERROR(dst < 0)) {
    rc = errno;
    goto done;
  }

  if (ERROR((rc = cp(dst, src))))
    goto done;

done:
  if (dst > 0)
    (void)close(dst);
  if (src > 0)
    (void)close(src);

  return rc;
}

int output_replay(const output_t output, const xc_trace_t *owner) {

  assert(output.path != NULL);
  assert(owner != NULL);

  int rc = 0;

  switch (output.tag) {

  case OUT_CHMOD:
    if (ERROR((rc = replay_chmod(output))))
      goto done;
    break;

  case OUT_CHOWN:
    if (ERROR((rc = replay_chown(output))))
      goto done;
    break;

  case OUT_MKDIR:
    if (ERROR((rc = replay_mkdir(output))))
      goto done;
    break;

  case OUT_WRITE:
    if (ERROR((rc = replay_write(output, owner))))
      goto done;
    break;
  }

done:
  return rc;
}
