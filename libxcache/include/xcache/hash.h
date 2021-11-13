#pragma once

#include <stdint.h>

/// A content hash used by xcache for associative lookups. Users should not rely
/// on the underlying type beyond it being a scalar.
typedef uint64_t xc_hash_t;
