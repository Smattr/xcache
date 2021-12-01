#pragma once

#include <stdio.h>

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// file manager interface
typedef struct {

  /// pointer to this object itself
  void *self;

  /// create a file for capturing a trace output into
  ///
  /// \param filem Self-pointer to the containing `xc_filem_t`
  /// \param fp [out] Write handle to the created file on success
  /// \param path [out] Path to the created file on success
  /// \return 0 on success or an errno on failure
  int (*mkfile)(void *filem, FILE **fp, char **path);
} xc_filem_t;

#ifdef __cplusplus
}
#endif
