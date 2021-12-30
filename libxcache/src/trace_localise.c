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

int trace_localise(const xc_db_t *db, xc_trace_t **local,
                   const xc_trace_t *global) {

  assert(local != NULL);
  assert(global != NULL);

  *local = NULL;
  int rc = -1;

  xc_trace_t *t = calloc(1, sizeof(*t));
  if (ERROR(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // setup space to copy the files accessed list
  t->io.base = calloc(global->io.size, sizeof(t->io.base[0]));
  if (ERROR(global->io.size > 0 && t->io.base == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  t->io.size = global->io.size;
  t->io.capacity = global->io.size;

  // copy the entries, making paths relative to the database root
  for (size_t i = 0; i < t->io.size; ++i) {
    fs_t *dst = &t->io.base[i];
    const fs_t *src = &global->io.base[i];
    assert(src->path != NULL);
    dst->path = strdup(src->path);
    if (ERROR(dst->path == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    dst->read = src->read;
    dst->existed = src->existed;
    dst->accessible = src->accessible;
    dst->written = src->written;
    dst->hash = src->hash;

    if (src->content_path != NULL) {
      assert(path_is_relative_to(src->content_path, db->root));
      size_t stem_offset = strlen(db->root) + 1 /* for '/' */;
      dst->content_path = strdup(src->content_path + stem_offset);
      if (ERROR(dst->content_path == NULL)) {
        rc = ENOMEM;
        goto done;
      }
    }
  }

  // success
  *local = t;
  t = NULL;
  rc = 0;

done:
  if (UNLIKELY(t != NULL)) {
    for (size_t i = 0; i < t->io.size; ++i) {
      free(t->io.base[i].path);
      free(t->io.base[i].content_path);
    }
    free(t->io.base);
  }
  free(t);

  return rc;
}
