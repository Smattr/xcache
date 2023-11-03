#include "proc_t.h"
#include <assert.h>

int proc_cont(const proc_t proc) {

  assert(proc.pid > 0);

  return proc_signal(proc, 0);
}
