#include "path.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool path_is_relative_to(const char *path, const char *root) {

  assert(path != NULL);
  assert(root != NULL);

  size_t len_root = strlen(root);
  if (strlen(path) < len_root)
    return false;

  if (strncmp(path, root, len_root) != 0)
    return false;

  if (path[len_root] != '/')
    return false;

  return true;
}
