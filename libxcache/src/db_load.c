#include "debug.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/db.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

int xc_db_load(xc_db_t *db, const xc_proc_t *question, xc_trace_t **answer) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(question == NULL))
    return EINVAL;

  if (ERROR(answer == NULL))
    return EINVAL;

  // TODO
  return ENOSYS;
}
