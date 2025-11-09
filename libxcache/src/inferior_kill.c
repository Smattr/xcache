#include "debug.h"
#include "inferior_t.h"
#include "list.h"
#include "thread_t.h"
#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <sys/wait.h>

void inferior_kill(inferior_t *inf) {

  assert(inf != NULL);

  // unceremoniously kill all threads
  for (size_t i = 0; i < LIST_SIZE(&inf->threads); ++i) {
    const thread_t *const t = LIST_AT(&inf->threads, i);
    if (t->id == 0)
      continue;
    (void)tgkill(t->proc->id, t->id, SIGKILL);
  }

  // reap them all
  for (size_t i = 0; i < LIST_SIZE(&inf->threads); ++i) {
    thread_t *const t = LIST_AT(&inf->threads, i);
    if (t->id == 0)
      continue;

    int status;
    if (ERROR(waitpid(t->id, &status, __WALL | __WNOTHREAD) < 0))
      continue; // FIXME?

    if (WIFEXITED(status)) {
      DEBUG("TID %ld exited with %d", (long)t->id, WEXITSTATUS(status));
      thread_exit(t, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      DEBUG("TID %ld died with signal %d", (long)t->id, WTERMSIG(status));
      thread_exit(t, 128 + WTERMSIG(status));
    }
    // FIXME: what if the child entered SIGSTOP due to still being ptraced?
  }
}
