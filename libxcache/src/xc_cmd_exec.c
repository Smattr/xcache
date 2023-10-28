#include "debug.h"
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <xcache/cmd.h>

int xc_cmd_exec(const xc_cmd_t cmd) {

  if (ERROR(cmd.argc == 0))
    return EINVAL;

  if (ERROR(cmd.argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < cmd.argc; ++i) {
    if (ERROR(cmd.argv[i] == NULL))
      return EINVAL;
  }

  // require a trailing `NULL` to match `execvp` semantics
  if (ERROR(cmd.argv[cmd.argc] != NULL))
    return EINVAL;

  int rc = 0;

  if (cmd.cwd != NULL) {
    if (ERROR(chdir(cmd.cwd) < 0)) {
      rc = errno;
      goto done;
    }
  }

  if (ERROR(execvp(cmd.argv[0], cmd.argv) < 0)) {
    rc = errno;
    goto done;
  }

done:
  return rc;
}
