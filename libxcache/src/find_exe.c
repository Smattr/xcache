#include "debug.h"
#include "find_me.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

/// `readlink`-alike that dynamically allocates
static int readln(const char *path, char **out) {

  char *resolved = NULL;
  size_t size = 512;
  int rc = 0;

  while (true) {

    // expand target buffer
    size *= 2;
    {
      char *r = realloc(resolved, size);
      if (r == NULL) {
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

int find_exe(char **exe) {

  assert(exe != NULL);

  *exe = NULL;
  char *path = NULL;
  int rc = 0;

  if (ERROR((rc = readln("/proc/self/exe", &path))))
    goto done;

  *exe = path;
  path = NULL;

done:
  free(path);

  return rc;
}
