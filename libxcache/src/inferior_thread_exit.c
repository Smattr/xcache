#include "inferior_t.h"
#include "list.h"
#include "thread_t.h"
#include <assert.h>
#include <stdlib.h>

void inferior_thread_exit(inferior_t *inf, thread_t *exiter, int exit_status) {
  assert(inf != NULL);
  assert(exiter != NULL);

  // remove the thread from our list of known threads
  for (size_t i = 0; i < LIST_SIZE(&inf->threads); ++i) {
    const thread_t *const candidate = *LIST_AT(&inf->threads, i);
    if (candidate != exiter)
      continue;
    LIST_POP(&inf->threads, i);
    break;
  }

  // cleanup the thread
  thread_exit(exiter, exit_status);
}
