#include "../../common/compiler.h"
#include "debug.h"
#include "fs.h"
#include "input_t.h"
#include "path.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// core logic of handling a `readlink*` call
///
/// @param inf Tracee executing `readlink*`
/// @param thread Thread executing `readlink*`
/// @param dirfd The `dirfd` parameter to `readlinkat`
/// @param path The `pathname` parameter to `readlink`/`readlinkat`
/// @return 0 on success or an errno on failure
static int core(inferior_t *inf, thread_t *thread, int dirfd,
                const char *path) {
  assert(inf != NULL);
  assert(thread != NULL);
  assert(path != NULL);

  char *root = NULL;
  char *abs = NULL;
  input_t saw = {0};
  int rc = 0;

  // extract the result
  const long ret = peek_ret(thread);
  const int err = peek_errno(thread);

  if (UNLIKELY(xc_debug != NULL)) {
    char *const fd_str = atfd_to_str(dirfd);
    DEBUG("TID %ld, readlinkat(%s, \"%s\", …) = %ld, errno == %d",
          (long)thread->id, fd_str == NULL ? "<oom>" : fd_str, path, ret,
          ret < 0 ? err : 0);
    free(fd_str);
  }

  // make the path absolute
  if (path[0] == '/') {
    // dirfd is ignored
    abs = strdup(path);
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else if (dirfd == AT_FDCWD) {
    if (strcmp(path, "") == 0) {
      abs = strdup(thread->fs->cwd);
    } else {
      abs = path_absolute(thread->fs->cwd, path);
    }
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else {
    const int r = thread_fd(thread, dirfd, &root);
    if (ERROR(r != 0)) {
      // consider the child passing an invalid file descriptor unsupported
      rc = (r == EINVAL || r == ENOENT) ? ECHILD : r;
      goto done;
    }
    if (strcmp(path, "") == 0) {
      abs = root;
      root = NULL;
    } else {
      abs = path_join(root, path);
      if (ERROR(abs == NULL)) {
        rc = ENOMEM;
        goto done;
      }
    }
  }

  // ignore reads of /proc/self/exe because we know this points somewhere
  // reliably reproducible, but will generate false negatives if we naïvely try
  // to replay it
  if (strcmp(abs, "/proc/self/exe") == 0) {
    DEBUG("ignoring readlink of \"/proc/self/exe\"");
    goto done;
  }

  // record it
  if (ERROR((rc = input_new_readlink(&saw, err, abs))))
    goto done;

  if (ERROR((rc = inferior_input_new(inf, saw))))
    goto done;
  saw = (input_t){0};

done:
  input_free(saw);
  free(abs);
  free(root);

  return rc;
}

int sysexit_readlink(inferior_t *inf, thread_t *thread) {
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

  if (ERROR((rc = core(inf, thread, AT_FDCWD, path))))
    goto done;

done:
  free(path);

  return rc;
}

int sysexit_readlinkat(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  char *path = NULL;
  int rc = 0;

  // extract the directory file descriptor
  const int dirfd = (int)peek_syscall_arg(thread, 1);

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_syscall_arg(thread, 2);
  if (ERROR((rc = peek_str(&path, thread->proc, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  if (ERROR((rc = core(inf, thread, dirfd, path))))
    goto done;

done:
  free(path);

  return rc;
}
