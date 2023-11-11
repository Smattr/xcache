#include "action_t.h"
#include <stdlib.h>

void action_free(action_t *action) {

  if (action == NULL)
    return;

  free(action->path);
  free(action);
}
