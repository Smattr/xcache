#include "../../common/compiler.h"
#include "debug.h"
#include "fs.h"
#include "inferior_t.h"
#include "input.h"
#include "output.h"
#include "path.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/record.h>

/// handle a sysexit of `open` or friends
///
/// @param inf Tracee
/// @param thread Tracee thread that made this call
/// @param abs_path Absolute path to target file being opened
/// @param flags `open` flags
/// @param err Any errno value set by the `open` call
/// @return 0 on success or an errno on failure
static int handle_open(inferior_t *inf, thread_t *thread, const char *abs_path,
                       int flags, int err) {
  assert(inf != NULL);
  assert(thread != NULL);
  assert(abs_path != NULL);
  assert(abs_path[0] == '/');

  input_t seen_read = {0};
  output_t seen_write = {0};
  int rc = 0;

  // discard the flags that have no relevance to us
  const long flags_relevant =
      flags & ~(O_APPEND | O_ASYNC | O_CLOEXEC | O_DIRECT | O_DSYNC |
                O_LARGEFILE | O_NOCTTY | O_NONBLOCK | O_NDELAY | O_SYNC);

  // ignore reads of some procfs files that we have effectively already
  // recorded through the command itself
  if (flags_relevant == O_RDONLY && path_is_ignorable(abs_path)) {
    DEBUG("ignoring open of \"%s\"", abs_path);
    goto done;
  }

  if (ERROR(!path_is_cacheable(abs_path))) {
    rc = ECHILD;
    goto done;
  }

  switch (flags_relevant) {

  case O_RDONLY:
  case O_RDONLY | O_CREAT:
  case O_RDONLY | O_CREAT | O_EXCL:
    // if this was an implied `creat`, this operation is actually semantically a
    // _write_
    if (thread->pending_creat) {
      assert(
          (flags_relevant & O_CREAT) &&
          "thread incurred pending creat from something not involving O_CREAT");

      // we can ignore failed implied `creat` because its effects were captured
      // during `sysenter_open`
      if (err != 0)
        break;

      // set a placeholder mode which will be updated later
      const mode_t mode = 0;

      // record it
      if (ERROR((rc = output_new_write(&seen_write, abs_path, mode))))
        goto done;

      if (ERROR((rc = inferior_output_new(inf, seen_write))))
        goto done;
      seen_write = (output_t){0};

      // if this was an attempted exclusive open that failed due to the file
      // pre-existing, the implied `access` in `sysenter_open` will have
      // captured this
    } else if ((flags_relevant & O_CREAT) && (flags_relevant & O_EXCL) &&
               err == EEXIST) {
      // nothing required
    } else {
      // record the read
      if (ERROR((rc = input_new_read(&seen_read, &err, abs_path))))
        goto done;

      if (ERROR((rc = inferior_input_new(inf, seen_read))))
        goto done;
      seen_read = (input_t){0};
    }

    break;

  case O_RDWR:
  case O_RDWR | O_CREAT:
  case O_RDWR | O_TRUNC:
  case O_RDWR | O_CREAT | O_TRUNC:
  case O_RDWR | O_CREAT | O_EXCL:
  case O_RDWR | O_CREAT | O_TRUNC | O_EXCL:
  case O_WRONLY:
  case O_WRONLY | O_CREAT:
  case O_WRONLY | O_TRUNC:
  case O_WRONLY | O_CREAT | O_TRUNC:
  case O_WRONLY | O_CREAT | O_EXCL:
  case O_WRONLY | O_CREAT | O_TRUNC | O_EXCL:

    // if we failed due to a non-existent, this is semantically an `access`
    if (!(flags_relevant & O_CREAT) && err == ENOENT) {
      if (ERROR((rc = input_new_access(&seen_read, &(int){ENOENT}, abs_path,
                                       F_OK, 0))))
        goto done;
      if (ERROR((rc = inferior_input_new(inf, seen_read))))
        goto done;
      seen_read = (input_t){0};
      break;
    }

    // if we failed due to an existent file, this will have been captured as an
    // `access` in `sysenter_open`
    if ((flags_relevant & O_EXCL) && err == EEXIST)
      break;

    // for now, do not support other categories of failing writes
    if (err != 0) {
      rc = ECHILD;
      goto done;
    }

    // set a placeholder mode which will be updated later
    const mode_t mode = 0;

    // record it
    if (ERROR((rc = output_new_write(&seen_write, abs_path, mode))))
      goto done;

    if (ERROR((rc = inferior_output_new(inf, seen_write))))
      goto done;
    seen_write = (output_t){0};

    break;

  default:
    // TODO
    rc = ENOTSUP;
    goto done;
  }

done:
  input_free(seen_read);
  output_free(seen_write);

  thread->pending_creat = false;

  return rc;
}

int sysexit_creat(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  char *pathname = NULL;
  char *abs_path = NULL;
  int rc = 0;

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_syscall_arg(thread, 1);
  if (ERROR((rc = peek_str(&pathname, thread->proc, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  // infer flags
  const long flags = O_CREAT | O_WRONLY | O_TRUNC;

  // extract the result
  const int err = peek_errno(thread);

  DEBUG("TID %ld, creat(\"%s\", …) = %ld, errno == %d", (long)thread->id,
        pathname, err == 0 ? peek_ret(thread) : -1, err);

  // make the path absolute
  if (pathname[0] == '/') {
    abs_path = pathname;
    pathname = NULL;
  } else {
    abs_path = path_absolute(thread->fs->cwd, pathname);
    if (ERROR(abs_path == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  if (ERROR((rc = handle_open(inf, thread, abs_path, flags, err))))
    goto done;

done:
  free(abs_path);
  free(pathname);

  return rc;
}

int sysexit_open(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  char *pathname = NULL;
  char *abs_path = NULL;
  int rc = 0;

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_syscall_arg(thread, 1);
  if (ERROR((rc = peek_str(&pathname, thread->proc, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  // extract the flags
  const long flags = peek_syscall_arg(thread, 2);

  // extract the result
  const int err = peek_errno(thread);

  if (UNLIKELY(xc_debug != NULL)) {
    char *flags_str = openflags_to_str(flags);
    DEBUG("TID %ld, open(\"%s\", %s, …) = %ld, errno == %d", (long)thread->id,
          pathname, flags_str == NULL ? "<oom>" : flags_str,
          err == 0 ? peek_ret(thread) : -1, err);
    free(flags_str);
  }

  // make the path absolute
  if (pathname[0] == '/') {
    abs_path = pathname;
    pathname = NULL;
  } else {
    abs_path = path_absolute(thread->fs->cwd, pathname);
    if (ERROR(abs_path == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  if (ERROR((rc = handle_open(inf, thread, abs_path, flags, err))))
    goto done;

done:
  free(abs_path);
  free(pathname);

  return rc;
}

int sysexit_openat(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  char *pathname = NULL;
  char *abs_path = NULL;
  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_syscall_arg(thread, 1);

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_syscall_arg(thread, 2);
  if (ERROR((rc = peek_str(&pathname, thread->proc, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  // extract the flags
  const long flags = peek_syscall_arg(thread, 3);

  // extract the result
  const int err = peek_errno(thread);

  if (UNLIKELY(xc_debug != NULL)) {
    char *fd_str = atfd_to_str(fd);
    char *flags_str = openflags_to_str(flags);
    DEBUG("TID %ld, openat(%s, \"%s\", %s, …) = %ld, errno == %d",
          (long)thread->id, fd_str == NULL ? "<oom>" : fd_str, pathname,
          flags_str == NULL ? "<oom>" : flags_str,
          err == 0 ? peek_ret(thread) : -1, err);
    free(flags_str);
    free(fd_str);
  }

  // make the path absolute
  if (pathname[0] == '/') {
    // fd is ignored
    abs_path = pathname;
    pathname = NULL;
  } else if (fd == AT_FDCWD) {
    abs_path = path_absolute(thread->fs->cwd, pathname);
    if (ERROR(abs_path == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else {
    // TODO
    rc = ENOTSUP;
    goto done;
  }

  if (ERROR((rc = handle_open(inf, thread, abs_path, flags, err))))
    goto done;

done:
  free(abs_path);
  free(pathname);

  return rc;
}
