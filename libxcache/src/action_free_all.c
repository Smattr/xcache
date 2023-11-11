#include "action_t.h"
#include <stddef.h>

void action_free_all(action_t *action) {
  while (action != NULL) {
    action_t *previous = action->previous;
    action_free(action);
    action = previous;
  }
}
