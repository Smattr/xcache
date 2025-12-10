#include "debug.h"
#include "path.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

int readln(const char *path, char **out) {

  char *resolved = NULL;
  size_t size = 512;
  int rc = 0;

  while (true) {

    // expand target buffer
    size *= 2;
    {
      char *r = realloc(resolved, size);
      if (ERROR(r == NULL)) {
        rc = ENOMEM;
        break;
      }
      resolved = r;
    }

    // attempt to resolve
    {
      ssize_t written = readlink(path, resolved, size);
      if (written < 0) {
        rc = errno;
        break;
      }
      if ((size_t)written < size) {
        // success
        resolved[written] = '\0';
        *out = resolved;
        resolved = NULL;
        break;
      }
    }
  }

  // failed
  free(resolved);
  return rc;
}
