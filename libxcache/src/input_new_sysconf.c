#include "debug.h"
#include "input.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int input_new_sysconf(input_t *input, int name) {
  assert(input != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  i.tag = INP_SYSCONF;

  // use an implementation-defined synthetic path
  if (ERROR(asprintf(&i.path, "//sysconf/%d", name) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  i.sysconf.name = name;
  i.sysconf.ret = sysconf(name);

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
