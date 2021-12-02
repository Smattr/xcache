#pragma once

#include "macros.h"
#include <stddef.h>

/// serialise a list of arguments to a string, returning NULL on ENOMEM
INTERNAL char *argv_serialise(size_t argc, char **argv);
