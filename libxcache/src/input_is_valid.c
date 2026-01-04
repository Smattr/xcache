#include "debug.h"
#include "input.h"
#include "path.h"
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

bool input_is_valid(const input_t input) {

  input_t attempt = {0};

  switch (input.tag) {

  case INP_ACCESS: {
    const int r = input_new_access(&attempt, NULL, input.path,
                                   input.access.mode, input.access.flags);
    if (r != 0)
      return false;
    break;
  }

  case INP_READ: {
    const int r = input_new_read(&attempt, NULL, input.path);
    if (r != 0)
      return false;
    break;
  }

  case INP_READLINK: {
    const int r = input_new_readlink(&attempt, NULL, input.path);
    if (r != 0)
      return false;
    break;
  }

  case INP_STAT: {
    const int r =
        input_new_stat(&attempt, NULL, input.path, input.stat.is_lstat);
    if (r != 0)
      return false;
    break;
  }

  case INP_SYSCONF: {
    const int r = input_new_sysconf(&attempt, input.sysconf.name);
    if (r != 0)
      return false;
    break;
  }

  case INP_GETENV: {
    const char *const value = getenv(input.getenv.key);
    const int r = input_new_getenv(&attempt, input.getenv.key, value);
    if (r != 0)
      return false;
    break;
  }

  case INP_UNLINK_PRE:
    // the target must exist and not be a directory
    {
      struct stat st;
      if (stat(input.path, &st) < 0)
        return false;
      if (S_ISDIR(st.st_mode))
        return false;
      return true;
    }
    // its parent must be writable
    {
      char *const parent = path_parent(input.path);
      if (ERROR(parent == NULL))
        return false;
      const bool is_writable = access(parent, W_OK) == 0;
      free(parent);
      if (!is_writable)
        return false;
    }
    return true;
    break;
  }

  const bool is_valid = input_eq(input, attempt);
  input_free(attempt);
  return is_valid;
}
