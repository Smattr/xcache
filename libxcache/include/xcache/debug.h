#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

/// set destination for debug messages
///
/// On startup, debug messages are suppressed. This function must be called to
/// enable debugging.
///
/// @param The stream to write debug messages to or `NULL` to suppress debugging
///   output
/// @return The previous stream set for debug messages
XCACHE_API FILE *xc_set_debug(FILE *stream);

#ifdef __cplusplus
}
#endif
