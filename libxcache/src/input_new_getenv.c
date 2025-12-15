#include "debug.h"
#include "input.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int input_new_getenv(input_t *input, const char *name, const char *value) {
  assert(input != NULL);
  assert(name != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  i.tag = INP_GETENV;

  // use an implementation-defined synthetic path
  if (ERROR(asprintf(&i.path, "//getenv/%s", name) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  i.getenv.key = strdup(name);
  if (ERROR(i.getenv.key == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (value != NULL) {
    i.getenv.value = strdup(value);
    if (ERROR(i.getenv.value == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
