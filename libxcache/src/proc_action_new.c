#include "action_t.h"
#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

int proc_action_new(proc_t *proc, const action_t action) {

  assert(proc != NULL);

  // do we need to expand the actions list?
  if (proc->n_actions == proc->c_actions) {
    const size_t c = proc->c_actions == 0 ? 1 : proc->c_actions * 2;
    action_t *a = realloc(proc->actions, c * sizeof(proc->actions[0]));
    if (ERROR(a == NULL))
      return ENOMEM;
    proc->c_actions = c;
    proc->actions = a;
  }

  assert(proc->n_actions < proc->c_actions);
  proc->actions[proc->n_actions] = action;
  ++proc->n_actions;

  return 0;
}
