#pragma once

#include <stddef.h>

struct xc_proc {

  /// command line parameters
  size_t argc;
  char **argv;

  /// current working directory
  char *cwd;
};
