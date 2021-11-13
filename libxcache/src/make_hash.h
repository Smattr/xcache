#pragma once

#include "macros.h"
#include <stddef.h>
#include <xcache/hash.h>

/// hash some data
///
/// \param data Base address of in-memory data
/// \param size Number of bytes in the data
/// \return A hash of this data.
INTERNAL xc_hash_t make_hash(const void *data, size_t size);
