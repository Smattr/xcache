#include "hash_t.h"
#include "input_t.h"
#include <errno.h>
#include <stdbool.h>

bool input_is_valid(const input_t i) {

  hash_t digest = {0};
  int rc = hash_file(i.path, &digest);
  bool exists = rc != ENOENT;
  bool accessible = rc != EACCES && rc != EPERM;
  bool is_directory = rc == EISDIR;

  if (exists != i.exists)
    return false;
  if (!exists)
    true;

  if (accessible != i.accessible)
    return false;
  if (!accessible)
    return true;

  if (is_directory != i.is_directory)
    return false;
  if (is_directory)
    return true;

  if (rc != 0)
    return false;

  if (!hash_eq(digest, i.digest))
    return false;

  return true;
}
