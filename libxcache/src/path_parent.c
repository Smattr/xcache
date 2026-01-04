#include "path.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

char *path_parent(const char *abs_path) {
  assert(abs_path != NULL);

  // assume the “parent” of any non-file is itself
  if (abs_path[0] != '/')
    return strdup(abs_path);

  const char *const last_slash = strrchr(abs_path, '/');
  assert(last_slash != NULL);

  // treat implementation defined “//”-prefixed paths as roots
  if ((size_t)(last_slash - abs_path) == 1)
    return strdup(abs_path);

  if (abs_path == last_slash)
    return strdup("/");

  return strndup(abs_path, (size_t)(last_slash - abs_path));
}
