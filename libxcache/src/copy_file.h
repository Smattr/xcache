#pragma once

#include "macros.h"

/// copy content from one file to another
///
/// \param src Path to source file
/// \param dst Path to destination file
/// \return 0 on success or an errno on failure
INTERNAL int copy_file(const char *src, const char *dst);
