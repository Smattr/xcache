#include "input_t.h"
#include <stdlib.h>

void input_free(input_t i) { free(i.path); }
