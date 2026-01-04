#include "debug.h"
#include "inferior_t.h"
#include "output.h"
#include "path.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int sysexit_unlink(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  char *path = NULL;
  char *abs_path = NULL;
  input_t i = {0};
  output_t o = {0};
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
  abs_path = path_absolute(thread->fs->cwd, path);
  if (ERROR(abs_path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // extract the result
  const int err = peek_errno(thread);

  DEBUG("TID %ld, unlink(\"%s\") = %d, errno == %d", (long)thread->id, path,
        err == 0 ? 0 : -1, err);

  // only handle successful unlink for now
  if (ERROR(err != 0)) {
    rc = ECHILD;
    goto done;
  }

  // note a dependency on the preconditions
  if (ERROR((rc = input_new_unlink_pre(&i, abs_path))))
    goto done;
  if (ERROR((rc = inferior_input_new(inf, i))))
    goto done;
  i = (input_t){0};

  // record this as an output
  if (ERROR((rc = output_new_unlink(&o, abs_path))))
    goto done;
  if (ERROR((rc = inferior_output_new(inf, o))))
    goto done;
  o = (output_t){0};

done:
  output_free(o);
  input_free(i);
  free(abs_path);
  free(path);

  return rc;
}
