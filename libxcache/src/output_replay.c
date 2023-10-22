#include "cp.h"
#include "output_t.h"
#include "trace_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xcache/trace.h>

static const mode_t PERMISSION_BITS = S_IRWXU | S_IRWXG | S_IRWXO;

static const mode_t CHMOD_BITS = PERMISSION_BITS | S_ISUID | S_ISGID | S_ISVTX;

static int replay_dir(const output_t output) {

  assert(S_ISDIR(output.st_mode));
  assert(output.cached_copy == NULL);

  int rc = 0;

  if (mkdir(output.path, output.st_mode & PERMISSION_BITS) < 0) {
    // allow directory to already exist
    if (errno != EEXIST) {
      rc = errno;
      goto done;
    }
  }

done:
  return rc;
}

static int replay_file(const output_t output, const xc_trace_t *owner) {

  assert(S_ISREG(output.st_mode));
  assert(output.cached_copy != NULL);

  int src = -1;
  int dst = -1;
  int rc = 0;

  src = openat(owner->root, output.cached_copy, O_RDONLY | O_CLOEXEC);
  if (src < 0) {
    rc = errno;
    goto done;
  }

  dst = open(output.path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
             output.st_mode & CHMOD_BITS);
  if (dst < 0) {
    rc = errno;
    goto done;
  }

  if ((rc = cp(dst, src)))
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
  assert((S_ISDIR(output.st_mode) && output.cached_copy == NULL) ||
         (S_ISREG(output.st_mode) && output.cached_copy != NULL));
  assert(owner != NULL);

  int rc = 0;

  // create the output
  if (S_ISDIR(output.st_mode)) {
    if ((rc = replay_dir(output)))
      goto done;
  } else {
    if ((rc = replay_file(output, owner)))
      goto done;
  }

  // read its properties
  struct stat st;
  if (stat(output.path, &st) < 0) {
    rc = errno;
    goto done;
  }

  // adjust any as needed
  if (st.st_mode != output.st_mode) {
    if (chmod(output.path, output.st_mode & CHMOD_BITS) < 0) {
      rc = errno;
      goto done;
    }
  }
  if (st.st_uid != output.st_uid || st.st_gid != output.st_gid) {
    if (chown(output.path, output.st_uid, output.st_gid) < 0) {
      rc = errno;
      goto done;
    }
  }

done:
  return rc;
}
