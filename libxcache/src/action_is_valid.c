#include "action_t.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

bool action_is_valid(const action_t action) {

  action_t attempt = {0};

  switch (action.tag) {

  case ACT_ACCESS: {
    const int r = action_new_access(&attempt, action.err, action.path,
                                    action.access.flags);
    if (r != 0)
      return false;
    break;
  }

  case ACT_READ: {
    const int r = action_new_read(&attempt, action.err, action.path);
    if (r != 0)
      return false;
    break;
  }

  case ACT_STAT: {
    const int r = action_new_stat(&attempt, action.err, action.path,
                                  action.stat.is_lstat);
    if (r != 0)
      return false;
    break;
  }

  // output actions
  case ACT_CREATE:
  case ACT_WRITE:
    return true;
  }

  const bool is_valid = action_eq(action, attempt);
  action_free(attempt);
  return is_valid;
}
