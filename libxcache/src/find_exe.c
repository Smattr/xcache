#include "debug.h"
#include "find_me.h"
#include "path.h"
#include <assert.h>
#include <stdlib.h>

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
