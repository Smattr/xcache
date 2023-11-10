#include "path.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

char *path_absolute(const char *cwd, const char *path) {

  assert(cwd != NULL);
  assert(path != NULL);

  // is it already absolute?
  if (path[0] == '/')
    return strdup(path);

  return path_join(cwd, path);
}
