#include "hash_t.h"
#include "input_t.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

bool input_is_valid(const input_t input) {

  assert(input.path != NULL);

  // read the target’s attributes
  {
    struct stat st = {0};
    if (stat(input.path, &st) < 0) {
      if (input.stat_errno != errno)
        return false;
    } else {
      if (input.stat_errno != 0)
        return false;
      if (input.st_mode != st.st_mode)
        return false;
      if (input.st_uid != st.st_uid)
        return false;
      if (input.st_gid != st.st_gid)
        return false;
      if (input.st_size != (size_t)st.st_size)
        return false;
    }
  }

  // hash it
  if (input.stat_errno == 0 && S_ISREG(input.st_mode) && input.st_size > 0) {
    hash_t digest = {0};
    int rc = hash_file(input.path, &digest);
    if (input.open_errno != rc)
      return false;
    if (rc == 0 && !hash_eq(input.digest, digest))
      return false;
  }

  return true;
}
