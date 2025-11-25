#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
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
#include <unistd.h>

int sysenter_openat(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  // In contrast to other syscalls that are intercepted in both sysenter and
  // sysexit, the behaviour of this function is _heavily_ coupled with the
  // behaviour of its other half, `sysexit_openat`. In particular it, (1)
  // returns 0 for some failure cases where it knows `sysexit_openat` will catch
  // the failure, (2) omits handling anything that `sysexit_openat` can handle,
  // and (3) omits handling anything that it knows will result in a `ECHILD` or
  // `ENOTSUP` from `sysexit_openat`.
  //
  // The relationship between `sysenter_openat` and `sysexit_openat` is
  // asymmetric. `sysenter_openat` is allowed to handle scenarios that
  // `sysexit_openat` rejects. But `sysenter_openat` is not allowed to skip
  // scenarios that `sysexit_openat` handles.

  char *path = NULL;
  char *abs = NULL;
  input_t input = {0};
  int rc = 0;

  // extract the flags first because this will allow us to skip the remaining
  // logic in the common cases
  const long flags = peek_syscall_arg(thread, 3);

  const long rw = flags & (O_RDONLY | O_WRONLY | O_RDWR);
  const bool is_wr = rw == O_WRONLY || rw == O_RDWR;
  const bool is_creat = !!(flags & O_CREAT);
  const bool is_trunc = !!(flags & O_TRUNC);

  // We have a number of possible relevant scenarios:
  //
  //   ┌────────────────────────────────────────────────────┬──────┐
  //   │ flags & (O_RDONLY|O_WRONLY|O_RDWR|O_CREAT|O_TRUNC) │ case │
  //   ├────────────────────────────────────────────────────┼──────┤
  //   │ O_RDONLY                                           │  1   │
  //   │ O_RDONLY | O_CREAT                                 │  2¹  │
  //   │ O_RDONLY |           O_TRUNC                       │  3   │
  //   │ O_RDONLY | O_CREAT | O_TRUNC                       │  3   │
  //   │ O_WRONLY                                           │  4   │
  //   │ O_WRONLY | O_CREAT                                 │  4   │
  //   │ O_WRONLY |           O_TRUNC                       │  1   │
  //   │ O_WRONLY | O_CREAT | O_TRUNC                       │  5   │
  //   │ O_RDWR                                             │  4   │
  //   │ O_RDWR   | O_CREAT                                 │  4   │
  //   │ O_RDWR             | O_TRUNC                       │  1   │
  //   │ O_RDWR   | O_CREAT | O_TRUNC                       │  5   │
  //   └────────────────────────────────────────────────────┴──────┘
  //
  //   1. Depends on the prior state of the file, but only in a way that does
  //      not need sysenter handling.
  //   2. Depends on the prior state of the file in a way that implies a
  //      `access(…, F_OK)` call.
  //   3. Unspecified behaviour. We can safely ignore these cases and let
  //      `sysexit_openat` bail out when seeing them.
  //   4. Depends on the prior state of the file in a way that implies a
  //      `open(…, O_RDONLY)` call.
  //   5. Does not depend on the prior state of the file.
  //
  // TODO: O_EXCL
  //
  // ¹ Bizarrely yes, `O_RDONLY|O_CREAT` is a legal flag combination.

  // nothing to be done for (1) cases
  if (rw == O_RDONLY && !is_creat && !is_trunc)
    goto done;
  if (is_wr && !is_creat && is_trunc)
    goto done;

  // the (2) cases will need to be dealt with later
  const bool implies_access = rw == O_RDONLY && is_creat;

  // ignore the (3) cases that `sysexit_openat` will reject
  if (rw == O_RDONLY && is_trunc)
    goto done;

  // the (4) cases will need to be dealt with later
  const bool implies_read = is_wr && !is_trunc;

  // nothing to be done for (5) cases
  if (is_wr && is_creat && is_trunc)
    goto done;

  // we should now have covered everything
  assert(implies_access || implies_read);

  // extract the file descriptor
  const int fd = (int)peek_syscall_arg(thread, 1);

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_syscall_arg(thread, 2);
  if (ERROR((rc = peek_str(&path, thread->proc, path_ptr)))) {
    // If the read faulted, assume our side was correct and the tracee used a
    // bad pointer. Leave this for `sysexit_open` to decide what to do.
    goto done;
  }

  // make the path absolute
  if (path[0] == '/') {
    // fd is ignored
    abs = path;
    path = NULL;
  } else if (fd == AT_FDCWD) {
    abs = path_absolute(thread->fs->cwd, path);
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else {
    // TODO
    rc = ENOTSUP;
    goto done;
  }

  // accrue the inferred check
  if (implies_access) {
    if (ERROR((rc = input_new_access(&input, NULL, abs, F_OK, 0))))
      goto done;
  } else {
    assert(implies_read);
    if (ERROR((rc = input_new_read(&input, NULL, abs))))
      goto done;
  }
  if (ERROR((rc = inferior_input_new(inf, input))))
    goto done;
  input = (input_t){0};

done:
  input_free(input);
  free(abs);
  free(path);

  return rc;
}
