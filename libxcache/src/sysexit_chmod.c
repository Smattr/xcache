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
#include <string.h>
#include <sys/stat.h>

static int handle(inferior_t *inf, thread_t *thread, const char *path) {
  assert(inf != NULL);
  assert(thread != NULL);
  assert(path != NULL);

  char *abs = NULL;
  output_t output = {0};
  int rc = 0;

  // make the path absolute
  if (path[0] == '/') {
    abs = strdup(path);
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else {
    if (strcmp(path, "") == 0) {
      abs = strdup(thread->fs->cwd);
    } else {
      abs = path_absolute(thread->fs->cwd, path);
    }
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  // instead of reading the mode, we use a placeholder that will be filled in
  // later
  const mode_t mode = 0;

  // record this
  if (ERROR((rc = output_new_chmod(&output, abs, mode))))
    goto done;
  if (ERROR((rc = inferior_output_new(inf, output))))
    goto done;
  output = (output_t){0};

done:
  output_free(output);
  free(abs);

  return rc;
}

int sysexit_chmod(inferior_t *inf, thread_t *thread) {
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

  // extract the result
  const int err = peek_errno(thread);

  DEBUG("TID %ld, chmod(\"%s\", …) = %ld, errno == %d", (long)thread->id, path,
        err == 0 ? peek_ret(thread) : -1, err);

  // do not support caching failed `chmod` for now
  if (ERROR(err != 0)) {
    rc = ECHILD;
    goto done;
  }

  if (ERROR((rc = handle(inf, thread, path))))
    goto done;

done:
  free(path);

  return rc;
}
