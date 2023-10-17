#include "debug.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/cmd.h>
#include <xcache/trace.h>

int xc_trace_record(xc_db_t *db, xc_cmd_t cmd, xc_trace_t **trace) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(cmd.argc == 0))
    return EINVAL;

  if (ERROR(cmd.argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < cmd.argc; ++i) {
    if (ERROR(cmd.argv[i] == NULL))
      return EINVAL;
  }

  if (ERROR(trace == NULL))
    return EINVAL;

  // TODO
  return ENOSYS;
}
