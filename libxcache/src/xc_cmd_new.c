#include "debug.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcache/cmd.h>

int xc_cmd_new(xc_cmd_t *cmd, size_t argc, char **argv, const char *cwd) {

  if (ERROR(cmd == NULL))
    return EINVAL;

  if (ERROR(argc == 0))
    return EINVAL;

  if (ERROR(argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < argc; ++i) {
    if (ERROR(argv[i] == NULL))
      return EINVAL;
  }

  *cmd = (xc_cmd_t){0};
  xc_cmd_t c = {0};
  int rc = 0;

  c.argv = calloc(argc + 1, sizeof(c.argv[0]));
  if (ERROR(c.argv == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  c.argc = argc;

  for (size_t i = 0; i < argc; ++i) {
    c.argv[i] = strdup(argv[i]);
    if (ERROR(c.argv[i] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  if (cwd == NULL) {
    c.cwd = getcwd(NULL, 0);
    if (ERROR(c.cwd == NULL)) {
      rc = errno;
      goto done;
    }
  } else {
    c.cwd = strdup(cwd);
    if (ERROR(c.cwd == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  *cmd = c;
  c = (xc_cmd_t){0};

done:
  xc_cmd_free(c);

  return rc;
}
