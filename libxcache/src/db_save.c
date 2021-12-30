#include "db.h"
#include "debug.h"
#include "fs_set.h"
#include "path.h"
#include "trace.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/db.h>
#include <xcache/trace.h>

int xc_db_save(xc_db_t *db, const xc_trace_t *observation) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(observation == NULL))
    return EINVAL;

  // reject the trace to be stored if it was not made using temporary files
  // rooted in this database’s store
  for (size_t i = 0; i < observation->io.size; ++i) {
    const fs_t *fs = &observation->io.base[i];
    if (fs->written) {
      if (ERROR(fs->content_path == NULL))
        return EINVAL;
      if (ERROR(!path_is_relative_to(fs->content_path, db->root)))
        return EINVAL;
    }
  }

  // TODO
  return ENOSYS;
}
