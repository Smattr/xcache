#pragma once

#include <stdio.h>

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// set output stream used for debugging messages
///
/// Passing `NULL` disables debugging messages. This is the mode libxcache
/// initially starts in.
///
/// \param stream New debugging stream to set
XCACHE_API void xc_set_debug(FILE *stream);

#ifdef __cplusplus
}
#endif
