#include "debug.h"
#include "input_t.h"
#include "path.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "inferior_t.h"

int sysexit_newfstatat(inferior_t *inf, proc_t *proc, thread_t *thread) {

  assert(inf != NULL);
  assert(proc != NULL);
  assert(thread != NULL);

  char *path = NULL;
  char *abs = NULL;
  input_t saw = {0};
  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_reg(thread, REG(rdi));

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_reg(thread, REG(rsi));
  if (ERROR((rc = peek_str(&path, proc, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  // extract the flags
  const long flags = peek_reg(thread, REG(r10));

  // extract the result
  const int err = peek_errno(thread);

  if (UNLIKELY(xc_debug != NULL)) {
    char *fd_str = atfd_to_str(fd);
    char *flags_str = statflags_to_str(flags);
    DEBUG("pid %ld, newfstatat(%s, \"%s\", …, %s) = %d, errno == %d",
          (long)thread->id, fd_str == NULL ? "<oom>" : fd_str, path,
          flags_str == NULL ? "<oom>" : flags_str, err == 0 ? 0 : -1, err);
    free(flags_str);
    free(fd_str);
  }

  // make the path absolute
  if (path[0] == '/') {
    // fd is ignored
    abs = path;
    path = NULL;
  } else if (fd == AT_FDCWD) {
    if (strcmp(path, "") == 0 && (flags & AT_EMPTY_PATH)) {
      abs = strdup(proc->cwd);
    } else {
      abs = path_absolute(proc->cwd, path);
    }
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else if (ERROR(fd < 0)) {
    rc = ECHILD;
    goto done;
  } else {
    const fd_t *dirfd = proc_fd(proc, fd);
    if (ERROR(dirfd == NULL)) {
      rc = ECHILD;
      goto done;
    }
    if (strcmp(path, "") == 0 && (flags & AT_EMPTY_PATH)) {
      abs = strdup(dirfd->path);
    } else {
      abs = path_join(dirfd->path, path);
    }
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  // record it
  {
    const bool is_lstat = !!(flags & AT_SYMLINK_NOFOLLOW);
    if (ERROR((rc = input_new_stat(&saw, err, abs, is_lstat))))
      goto done;
  }

  if (ERROR((rc = inferior_input_new(inf, saw))))
    goto done;
  saw = (input_t){0};

done:
  input_free(saw);
  free(abs);
  free(path);

  return rc;
}
