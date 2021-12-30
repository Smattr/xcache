#include "debug.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/db.h>
#include <xcache/trace.h>

int xc_db_save(xc_db_t *db, const xc_trace_t *observation) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(observation == NULL))
    return EINVAL;

  // TODO
  return ENOSYS;
}
