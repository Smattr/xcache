#include "debug.h"
#include "path.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *path_join(const char *a, const char *b) {

  assert(a != NULL);
  assert(strcmp(a, "") != 0);
  assert(b != NULL);

  // is `b` already an absolute path?
  if (b[0] == '/')
    return strdup(b);

  // will we need to insert an extra '/' in between `a` and `b`?
  bool slash = a[strlen(a) - 1] != '/' && strcmp(b, "") != 0;

  size_t len = strlen(a) + (slash ? 1 : 0) + strlen(b) + 1;
  char *result = malloc(len);
  if (ERROR(result == NULL))
    return NULL;

  snprintf(result, len, "%s%s%s", a, slash ? "/" : "", b);

  return result;
}
