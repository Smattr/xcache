#include "debug.h"
#include "fd.h"
#include "fs.h"
#include "inferior_t.h"
#include "input_t.h"
#include "path.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcache/record.h>

/// convert `access` mode to a readable string
static char *mode_to_str(long mode) {

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  char *ret = NULL;

  stream = open_memstream(&buffer, &buffer_size);
  if (ERROR(stream == NULL))
    goto done;

  const struct {
    const char *name;
    long value;
  } KNOWN[] = {
#define X(v) {#v, v}
      X(R_OK),
      X(W_OK),
      X(X_OK),
      X(F_OK),
#undef X
  };
  const char *separator = "";
  for (size_t i = 0; i < sizeof(KNOWN) / sizeof(KNOWN[0]); ++i) {
    if (!(mode & KNOWN[i].value))
      continue;
    if (fprintf(stream, "%s%s", separator, KNOWN[i].name) < 0)
      goto done;
    mode &= ~KNOWN[i].value;
    separator = "|";
  }

  if (mode != 0) {
    if (fprintf(stream, "%s%ld", separator, mode) < 0)
      goto done;
  }

  (void)fclose(stream);
  stream = NULL;
  ret = buffer;
  buffer = NULL;

done:
  if (stream != NULL)
    (void)fclose(stream);
  free(buffer);

  return ret;
}

/// handle a call to an `access` alike by the tracee
///
/// @param inf Inferior that made the call
/// @param thread Thread that made the call
/// @param dirfd Directory from which `pathname` is relative
/// @param pathname Path passed to `access`
/// @param mode Mode passed to `access`
/// @param flags Flags to an `faccessat2` call
/// @return 0 on success or an errno on failure
static int handle_access(inferior_t *inf, thread_t *thread, int dirfd,
                         const char *pathname, long mode, long flags) {
  assert(inf != NULL);
  assert(thread != NULL);
  assert(pathname != NULL);

  char *abs = NULL;
  input_t saw = {0};
  int rc = 0;

  // make the path absolute
  if (pathname[0] == '/') {
    // dirfd is ignored
    abs = strdup(pathname);
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else if (dirfd == AT_FDCWD) {
    abs = path_absolute(thread->fs->cwd, pathname);
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else {
    const fd_t *const root = fd_at(thread->fd, dirfd);
    if (ERROR(root == NULL)) {
      rc = ECHILD;
      goto done;
    }
    abs = path_join(root->path, pathname);
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  // extract the result
  const int err = peek_errno(thread);

  if (UNLIKELY(xc_debug != NULL)) {
    char *const mode_str = mode_to_str(mode);
    if (dirfd == AT_FDCWD && flags == 0) {
      DEBUG("TID %ld, access(\"%s\", %s) = %d, errno == %d", (long)thread->id,
            pathname, mode_str == NULL ? "<oom>" : mode_str, err == 0 ? 0 : -1,
            err);
    } else {
      DEBUG("TID %ld, faccessat2(%d, \"%s\", %s, %ld) = %d, errno == %d",
            (long)thread->id, dirfd, abs, mode_str == NULL ? "<oom>" : mode_str,
            flags, err == 0 ? 0 : -1, err);
    }
    free(mode_str);
  }

  // treat any mode flag we do not know as the child doing something unsupported
  if (ERROR(mode & ~(R_OK | W_OK | X_OK | F_OK))) {
    rc = ECHILD;
    goto done;
  }

  // record it
  if (ERROR((rc = input_new_access(&saw, &err, abs, (int)mode))))
    goto done;

  if (ERROR((rc = inferior_input_new(inf, saw))))
    goto done;
  saw = (input_t){0};

done:
  input_free(saw);
  free(abs);

  return rc;
}

int sysexit_access(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  char *path = NULL;
  int rc = 0;

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_syscall_arg(thread, 1);
  if (ERROR((rc = peek_str(&path, thread->proc, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  // extract the mode flags
  const long mode = peek_syscall_arg(thread, 2);

  if (ERROR((rc = handle_access(inf, thread, AT_FDCWD, path, mode, 0))))
    goto done;

done:
  free(path);

  return rc;
}
