#pragma once

#include "macros.h"

/// convert a SQLite result code to a Linux errno
INTERNAL int sqlite_error_to_errno(int err);
