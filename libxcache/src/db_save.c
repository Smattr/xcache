#include "db.h"
#include "debug.h"
#include "fs_set.h"
#include "pack.h"
#include "path.h"
#include "trace.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcache/db.h>
#include <xcache/hash.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

int xc_db_save(xc_db_t *db, const xc_proc_t *proc,
               const xc_trace_t *observation) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(proc == NULL))
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

  char *output = NULL;
  xc_trace_t *localised = NULL;
  FILE *f = NULL;
  int rc = -1;

  // figure out where we will save this trace
  xc_hash_t hash = 0;
  rc = xc_hash_proc(&hash, proc);
  if (ERROR(rc != 0))
    goto done;
  char hash_str[sizeof(hash) * 2 + 1];
  snprintf(hash_str, sizeof(hash_str), "%" PRIxHASH, hash);
  output = path_join(db->root, hash_str);
  if (ERROR(output == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // localise the trace, so the database contents are relocatable
  rc = trace_localise(db, &localised, observation);
  if (ERROR(rc != 0))
    goto done;

  // save the entry itself
  f = fopen(output, "w");
  if (ERROR(f == NULL)) {
    rc = errno;
    goto done;
  }
  rc = pack_proc(f, proc);
  if (ERROR(rc != 0))
    goto done;
  rc = pack_trace(f, localised);
  if (ERROR(rc != 0))
    goto done;
  {
    int r = fclose(f);
    f = NULL;
    if (ERROR(r == EOF)) {
      rc = errno;
      goto done;
    }
  }

  // success
  rc = 0;

done:
  if (f != NULL)
    (void)fclose(f);
  if (rc != 0 && output != NULL)
    (void)unlink(output);
  if (localised != NULL)
    trace_deinit(localised);
  free(output);

  return rc;
}
