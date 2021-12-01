#pragma once

#include "macros.h"

/// join two path components to form their concatenation
///
/// \return The concatenation on success or NULL on ENOMEM
INTERNAL char *path_join(const char *a, const char *b);
