#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

/** get the version of this library
 *
 * Release versions (see `xc_version_is_release`) can be compared for order
 * with `strcmp`. Other versions can only be compared for equality with
 * `strcmp`.
 *
 * \return Version string of this library
 */
XCACHE_API const char *xc_version(void);

/** is the given version a release?
 *
 * Non-release versions are development versions built from Git snapshots.
 *
 * \param version A version string to evaluate
 * \return True if this is a release version
 */
XCACHE_API bool xc_version_is_release(const char *version);

#ifdef __cplusplus
}
#endif
