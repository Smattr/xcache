#include "path.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

size_t path_parent(const char *abs_path, size_t len) {
  assert(abs_path != NULL);
  assert(len > 0);

  // assume the “parent” of any non-file is itself
  if (abs_path[0] != '/')
    return len;

  const char *const last_slash = memrchr(abs_path, '/', len);
  assert(last_slash != NULL);

  // treat implementation defined “//”-prefixed paths as roots
  if ((size_t)(last_slash - abs_path) == 1)
    return len;

  if (abs_path == last_slash)
    return 1; // “/”

  return (size_t)(last_slash - abs_path);
}
