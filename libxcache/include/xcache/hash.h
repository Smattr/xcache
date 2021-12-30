#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <xcache/proc.h>

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// A content hash used by xcache for associative lookups. Users should not rely
/// on the underlying type beyond it being a scalar.
typedef uint64_t xc_hash_t;
#define PRIxHASH PRIx64

/// compute the hash of the given in-memory data
///
/// \param hash [out] On success, computed hash value
/// \param data Base of in-memory data
/// \param size Number of bytes in data
/// \return 0 on success or an errno on failure
XCACHE_API int xc_hash_data(xc_hash_t *hash, const void *data, size_t size);

/// compute the hash of the contents of the given file
///
/// \param hash [out] On success, computed hash value
/// \param path File whose content to hash
/// \return 0 on success or an errno on failure
XCACHE_API int xc_hash_file(xc_hash_t *hash, const char *path);

/// compute the has of a process
///
/// \param hash [out] On success, computed hash value
/// \param proc Process to hash
/// \return 0 on success or an errno on failure
XCACHE_API int xc_hash_proc(xc_hash_t *hash, const xc_proc_t *proc);

#ifdef __cplusplus
}
#endif
