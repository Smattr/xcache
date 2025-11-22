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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcache/record.h>

/// convert `access` mode to a readable string
static char *mode_to_str(int mode) {

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  char *ret = NULL;

  stream = open_memstream(&buffer, &buffer_size);
  if (ERROR(stream == NULL))
    goto done;

  const struct {
    const char *name;
    int value;
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
    if (fprintf(stream, "%s%d", separator, mode) < 0)
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

  if (UNLIKELY(xc_debug != NULL)) {
    char *const mode = mode_to_str((int)flags);
    DEBUG("TID %ld, access(\"%s\", %s) = %d, errno == %d", (long)thread->id,
          path, mode == NULL ? "<oom>" : mode, err == 0 ? 0 : -1, err);
    free(mode);
  }

  // record it
  if (ERROR((rc = input_new_access(&saw, &err, abs, (int)flags))))
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
