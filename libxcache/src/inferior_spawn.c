#include "debug.h"
#include "fd.h"
#include "fs.h"
#include "inferior_t.h"
#include "list.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

int inferior_spawn(inferior_t *inf, thread_t *parent, pid_t child) {
  assert(inf != NULL);
  assert(parent != NULL);
  assert(parent->id != child && "thread claims to be spawning itself");

  thread_t *new = NULL;
  int rc = 0;

  if (ERROR(!parent->clone_flags.set)) {
    DEBUG("TID %ld entered clone without first calling a clone syscall",
          (long)parent->id);
    rc = ECHILD;
    goto done;
  }

  new = calloc(1, sizeof(*new));
  if (ERROR(new == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  new->id = child;

  // will the child have the same thread group (process ID) as the parent?
  if (parent->clone_flags.clone_thread) {
    new->proc = parent->proc;
    ++new->proc->reference_count;
  } else {
    new->proc = calloc(1, sizeof(*new->proc));
    if (ERROR(new->proc == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    ++new->proc->reference_count;
  }

  // will the child have the same filesystem information as the parent?
  if (parent->clone_flags.clone_fs) {
    new->fs = fs_acquire(parent->fs);
  } else {
    new->fs = fs_dup(parent->fs);
    if (ERROR(new->fs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  // will the child have the same file descriptor table as the parent?
  if (parent->clone_flags.clone_files) {
    new->fd = fds_acquire(parent->fd);
  } else {
    new->fd = fds_dup(parent->fd);
    if (ERROR(new->fd == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  if (ERROR((rc = LIST_PUSH_BACK(&inf->threads, new))))
    goto done;
  new = NULL;

done:
  if (new != NULL)
    thread_exit(new, 0);

  // mark the clone flags as consumed
  parent->clone_flags = (clone_flags_t){0};

  return rc;
}
