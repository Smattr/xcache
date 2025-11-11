#include "../../common/compiler.h"
#include "debug.h"
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

int sysexit_readlinkat(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  char *abs = NULL;
  char *path = NULL;
  input_t saw = {0};
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
    abs = path;
    path = NULL;
  } else if (dirfd == AT_FDCWD) {
    if (strcmp(path, "") == 0) {
      abs = strdup(thread->proc->cwd);
    } else {
      abs = path_absolute(thread->proc->cwd, path);
    }
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else {
    const fd_t *fd = proc_fd(thread->proc, dirfd);
    if (ERROR(fd == NULL)) {
      rc = ECHILD;
      goto done;
    }
    if (strcmp(path, "") == 0) {
      abs = strdup(fd->path);
    } else {
      abs = path_join(fd->path, path);
    }
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  // record it
  // FIXME: this is wrong. This reads the target of the link, but we actually
  // want to read the link itself.
  if (ERROR((rc = input_new_read(&saw, err, abs))))
    goto done;

  if (ERROR((rc = inferior_input_new(inf, saw))))
    goto done;
  saw = (input_t){0};

done:
  input_free(saw);
  free(abs);
  free(path);

  return rc;
}
