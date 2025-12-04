/// @file
/// @brief Interface to a set of strings
///
/// This set implementation is pretty standard except for one wrinkle. A set can
/// contain the “universe”, a symbolic representation of every possible string.

#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>
#include <stddef.h>

/// a set of strings
typedef struct {
  char **data;    ///< backing allocation for stored items
  size_t size;    ///< number of items in the set
  size_t buckets; ///< number of slots in `data` array
} set_t;

/// insert an item into a set
///
/// @param set Set to operate on
/// @param item Item to insert
/// @return 0 on success or an errno on failure
INTERNAL int set_add(set_t *set, const char *item);

/// add every possible string to a set
///
/// @param set Set to operate on
/// @return 0 on success or an errno on failure
INTERNAL int set_add_universe(set_t *set);

/// does an item exist within a set?
///
/// @param set Set to operate on
/// @param item Item to look for
/// @return True if the item is found
INTERNAL bool set_contains(const set_t *set, const char *item);

/// duplicate a set
///
/// @param dst [out] Copied set on success
/// @param src Set to copy
/// @return 0 on success or an errno on failure
INTERNAL int set_copy(set_t *dst, const set_t *src);

/// deallocate resources for a set
///
/// @param set Set to operate on
INTERNAL void set_free(set_t set);
