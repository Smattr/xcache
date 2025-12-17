#include "debug.h"
#include "inferior_t.h"
#include "input.h"
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
#include <unistd.h>

/// handle a sysenter of `open` or friends
///
/// @param inf Tracee
/// @param thread Tracee thread that made this call
/// @param abs Absolute path to target file being opened
/// @param flags `open` flags
/// @return 0 on success or an errno on failure
static int handle_open(inferior_t *inf, thread_t *thread, const char *abs,
                       int flags) {
  assert(inf != NULL);
  assert(thread != NULL);

  // In contrast to other syscalls that are intercepted in both sysenter and
  // sysexit, the behaviour of this function is _heavily_ coupled with the
  // behaviour of its other half, `sysexit_openat`. In particular it, (1)
  // returns 0 for some failure cases where it knows `sysexit_openat` will catch
  // the failure, (2) omits handling some things that `sysexit_openat` can
  // handle, and (3) omits handling anything that it knows will result in a
  // `ECHILD` or `ENOTSUP` from `sysexit_openat`.
  //
  // The relationship between `sysenter_openat` and `sysexit_openat` is
  // asymmetric. `sysenter_openat` is allowed to handle scenarios that
  // `sysexit_openat` rejects. But `sysenter_openat` is not allowed to skip
  // scenarios that `sysexit_openat` handles.

  input_t input = {0};
  int rc = 0;

  const long rw = flags & (O_RDONLY | O_WRONLY | O_RDWR);
  const bool is_wr = rw == O_WRONLY || rw == O_RDWR;
  const bool is_creat = !!(flags & O_CREAT);
  const bool is_trunc = !!(flags & O_TRUNC);
  const bool is_excl = !!(flags & O_EXCL);

  // We have a number of possible relevant scenarios:
  //
  //   ┌───────────────────────────────────────────────────────────┬──────────┐
  //   │ flags & (O_RDONLY|O_WRONLY|O_RDWR|O_CREAT|O_TRUNC|O_EXCL) │   case   │
  //   ├───────────────────────────────────────────────────────────┼──────────┤
  //   │ O_RDONLY                                                  │    N/A   │
  //   │ O_RDONLY | O_CREAT                                        │   F_OK¹  │
  //   │ O_RDONLY |           O_TRUNC                              │  unspec  │
  //   │ O_RDONLY | O_CREAT | O_TRUNC                              │  unspec  │
  //   │ O_RDONLY |                     O_EXCL                     │    UB²   │
  //   │ O_RDONLY | O_CREAT |           O_EXCL                     │ F_OK (2)¹│
  //   │ O_RDONLY |           O_TRUNC | O_EXCL                     │    UB²   │
  //   │ O_RDONLY | O_CREAT | O_TRUNC | O_EXCL                     │  unspec  │
  //   │ O_WRONLY                                                  │ O_RDONLY │
  //   │ O_WRONLY | O_CREAT                                        │ O_RDONLY │
  //   │ O_WRONLY |           O_TRUNC                              │ F_OK (2) │
  //   │ O_WRONLY | O_CREAT | O_TRUNC                              │   none   │
  //   │ O_WRONLY |                     O_EXCL                     │    UB²   │
  //   │ O_WRONLY | O_CREAT |           O_EXCL                     │ F_OK (2) │
  //   │ O_WRONLY |           O_TRUNC | O_EXCL                     │    UB²   │
  //   │ O_WRONLY | O_CREAT | O_TRUNC | O_EXCL                     │ F_OK (2) │
  //   │ O_RDWR                                                    │ O_RDONLY │
  //   │ O_RDWR   | O_CREAT                                        │ O_RDONLY │
  //   │ O_RDWR   |           O_TRUNC                              │ F_OK (2) │
  //   │ O_RDWR   | O_CREAT | O_TRUNC                              │   none   │
  //   │ O_RDWR   |                     O_EXCL                     │    UB²   │
  //   │ O_RDWR   | O_CREAT |           O_EXCL                     │ F_OK (2) │
  //   │ O_RDWR   |           O_TRUNC | O_EXCL                     │    UB²   │
  //   │ O_RDWR   | O_CREAT | O_TRUNC | O_EXCL                     │ F_OK (2) │
  //   └───────────────────────────────────────────────────────────┴──────────┘
  //
  //   [N/A]
  //     Depends on the prior state of the file, but only in a way that does not
  //     need sysenter handling.
  //
  //   [F_OK]
  //     Depends on the prior state of the file in a way that implies a
  //     `access(…, F_OK)` call.
  //
  //   [unspec]
  //     Unspecified behaviour. We can safely ignore these cases and let
  //     `sysexit_openat` bail out when seeing them.
  //
  //   [UB]
  //     Undefined behaviour. We can safely ignore these cases and let
  //     `sysexit_openat` bail out when seeing them.
  //
  //   [F_OK (2)]
  //     Depends on the prior state of the file in a way that implies a
  //     `access(…, F_OK)` call but technically can be inferred by
  //     `sysexit_openat`. Still, it is easier to handle here.
  //
  //   [O_RDONLY]
  //     Depends on the prior state of the file in a way that implies a
  //     `open(…, O_RDONLY)` call.
  //
  //   [none]
  //     Does not depend on the prior state of the file.
  //
  // ¹ Bizarrely yes, `O_RDONLY|O_CREAT` is a legal flag combination.
  // ² There is an exception where `O_EXCL` without `O_CREAT` is defined on
  //   block devices. But we reject this in `sysexit_openat`.

  // nothing to be done for [N/A] cases
  if (rw == O_RDONLY && !is_creat && !is_trunc && !is_excl)
    goto done;

  // the [F_OK] cases and [F_OK (2)] cases will need to be dealt with later
  bool implies_access = false;
  implies_access |= rw == O_RDONLY && is_creat && !is_trunc;
  implies_access |= is_wr && !is_creat && is_trunc && !is_excl;
  implies_access |= is_wr && is_creat && is_excl;

  // ignore the [unspec] cases that `sysexit_openat` will reject
  if (rw == O_RDONLY && is_trunc)
    goto done;

  // ignore the [UB] cases that `sysexit_openat` will reject
  if (!is_creat && is_excl)
    goto done;

  // the [O_RDONLY] cases will need to be dealt with later
  const bool implies_read = is_wr && !is_trunc && !is_excl;

  // nothing to be done for [none] cases
  if (is_wr && is_creat && is_trunc && !is_excl)
    goto done;

  // we should now have covered everything
  assert(implies_access || implies_read);

  // ignore reads of some procfs files that we have effectively already recorded
  // through the command itself
  if (path_is_ignorable(abs)) {
    DEBUG("ignoring open of \"%s\"", abs);
    goto done;
  }

  if (ERROR(!path_is_cacheable(abs))) {
    rc = ECHILD;
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

  // as a bonus, the bizarre combination of `O_RDONLY|O_CREAT` is legal and we
  // need to handle it here because, by the time of `sysexit_open`, the
  // pre-existence of the target file can no longer be determined
  assert(!thread->pending_creat);
  if (rw == O_RDONLY && is_creat && access(abs, F_OK) < 0)
    thread->pending_creat = true;

done:
  input_free(input);

  return rc;
}

int sysenter_openat(inferior_t *inf, thread_t *thread) {

  char *pathname = NULL;
  char *abs_path = NULL;
  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_syscall_arg(thread, 1);

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_syscall_arg(thread, 2);
  if (ERROR((rc = peek_str(&pathname, thread->proc, path_ptr)))) {
    // If the read faulted, assume our side was correct and the tracee used a
    // bad pointer. Leave this for `sysexit_open` to decide what to do.
    goto done;
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

  // extract the flags
  const long flags = peek_syscall_arg(thread, 3);

  if (ERROR((rc = handle_open(inf, thread, abs_path, flags))))
    goto done;

done:
  free(abs_path);
  free(pathname);

  return rc;
}
