#include "db.h"
#include "debug.h"
#include "fs_set.h"
#include "macros.h"
#include "path.h"
#include "trace.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/db.h>
#include <xcache/trace.h>

int trace_globalise(const xc_db_t *db, xc_trace_t **global,
                    const xc_trace_t *local) {

  assert(db != NULL);
  assert(global != NULL);
  assert(local != NULL);

  *global = NULL;
  int rc = -1;

  xc_trace_t *t = calloc(1, sizeof(*t));
  if (ERROR(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // setup space to copy the files accessed list
  t->io.base = calloc(local->io.size, sizeof(t->io.base[0]));
  if (ERROR(local->io.size > 0 && t->io.base == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  t->io.size = local->io.size;
  t->io.capacity = local->io.size;

  // copy the entries, making paths absolute
  for (size_t i = 0; i < t->io.size; ++i) {
    fs_t *dst = &t->io.base[i];
    const fs_t *src = &local->io.base[i];
    assert(src->path != NULL);
    dst->path = strdup(src->path);
    if (ERROR(dst->path == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    dst->read = src->read;
    dst->existed = src->existed;
    dst->accessible = src->accessible;
    dst->is_directory = src->is_directory;
    dst->written = src->written;
    dst->hash = src->hash;

    if (src->content_path != NULL) {
      dst->content_path = path_join(db->root, src->content_path);
      if (ERROR(dst->content_path == NULL)) {
        rc = ENOMEM;
        goto done;
      }
    }
  }

  // copy the exit status, as-is
  t->exit_status = local->exit_status;

  // success
  *global = t;
  t = NULL;
  rc = 0;

done:
  xc_trace_free(t);

  return rc;
}
