#include "action_t.h"
#include <stdlib.h>

void action_free(action_t action) { free(action.path); }
