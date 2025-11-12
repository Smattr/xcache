#include "input_t.h"
#include <stdbool.h>

bool input_is_valid(const input_t input) {

  input_t attempt = {0};

  switch (input.tag) {

  case INP_ACCESS: {
    const int r =
        input_new_access(&attempt, input.err, input.path, input.access.flags);
    if (r != 0)
      return false;
    break;
  }

  case INP_READ: {
    const int r = input_new_read(&attempt, input.err, input.path);
    if (r != 0)
      return false;
    break;
  }

  case INP_READLINK: {
    const int r = input_new_readlink(&attempt, input.err, input.path);
    if (r != 0)
      return false;
    break;
  }

  case INP_STAT: {
    const int r =
        input_new_stat(&attempt, input.err, input.path, input.stat.is_lstat);
    if (r != 0)
      return false;
    break;
  }
  }

  const bool is_valid = input_eq(input, attempt);
  input_free(attempt);
  return is_valid;
}
