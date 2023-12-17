#include "proc_t.h"
#include <assert.h>

int thread_cont(thread_t thread) {

  assert(thread.id > 0);

  return thread_signal(thread, 0);
}
