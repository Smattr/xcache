#include "inferior_t.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

/// common inner handling logic
static void core(thread_t *thread) {
  assert(thread != NULL);

  // Note that this thread no longer has a pending clone, regardless of whether
  // the `clone`/`clone3` actually succeeded. This is really only necessary in
  // the case where the`clone`/`clone3` failed, leaving us having never seen a
  // `PTRACE_EVENT_CLONE` but the thread still having clone flags saved.
  thread->clone_flags = (clone_flags_t){0};
}

int sysexit_clone(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  (void)inf;

  core(thread);

  return 0;
}

int sysexit_clone3(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  (void)inf;

  core(thread);

  return 0;
}
