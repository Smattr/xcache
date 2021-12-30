#pragma once

#include "macros.h"
#include <stdbool.h>

/// join two path components to form their concatenation
///
/// \return The concatenation on success or NULL on ENOMEM
INTERNAL char *path_join(const char *a, const char *b);

/// is this path relative to the given root?
INTERNAL bool path_is_relative_to(const char *path, const char *root);
