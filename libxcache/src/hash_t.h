#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** summary digest of some data
 *
 * Users should treat this as an opaque non-scalar type, and not rely on its
 * internals.
 */
typedef struct {
  uint64_t data;
} hash_t;

/// compare two hashes for equality
INTERNAL bool hash_eq(hash_t a, hash_t b);

/** derive the hash of in-memory data
 *
 * \param base Start of the data
 * \param size Number of bytes to read
 * \return Hash of the data
 */
INTERNAL hash_t hash_data(const void *base, size_t size);

/** derive the hash of a given file
 *
 * \param path File to examine
 * \param hash [out] Hash value on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int hash_file(const char *path, hash_t *hash);

/** deserialise a hash value from a file
 *
 * \param hash [out] Reconstructed hash on success
 * \param stream File to read from
 * \return 0 on success or an errno on failure
 */
INTERNAL int hash_read(hash_t *hash, FILE *stream);
