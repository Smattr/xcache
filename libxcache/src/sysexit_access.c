#include "debug.h"
#include "fs.h"
#include "inferior_t.h"
#include "input_t.h"
#include "path.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcache/record.h>

int sysexit_access(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  char *path = NULL;
  char *abs = NULL;
  input_t saw = {0};
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

  // make it absolute
  abs = path_absolute(thread->fs->cwd, path);
  if (ERROR(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // extract the flags
  const long flags = peek_syscall_arg(thread, 2);

  // treat any flag we do not know as the child doing something unsupported
  if (ERROR(flags & ~(R_OK | W_OK | X_OK | F_OK))) {
    rc = ECHILD;
    goto done;
  }

  // extract the result
  const int err = peek_errno(thread);

  // TODO: translate the flags. Reading them numerically is annoying.
  DEBUG("TID %ld, access(\"%s\", %ld) = %d, errno == %d", (long)thread->id,
        path, flags, err == 0 ? 0 : -1, err);

  // record it
  if (ERROR((rc = input_new_access(&saw, err, abs, (int)flags))))
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
