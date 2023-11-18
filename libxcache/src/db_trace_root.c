#include "db_t.h"
#include "debug.h"
#include "hash_t.h"
#include "path.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcache/cmd.h>
#include <xcache/db.h>

int db_trace_root(const xc_db_t db, const xc_cmd_t cmd, char **root) {

  assert(root != NULL);

  *root = NULL;
  char *trace_root = NULL;
  int rc = 0;

  // hash the command
  hash_t hash = {0};
  if (ERROR((rc = hash_cmd(cmd, &hash))))
    goto done;

  // stringise the hash
  char stem[sizeof(hash_t) * 2 + 1] = {0};
  snprintf(stem, sizeof(stem), "%016" PRIx64, hash.data);

  // construct a path to this trace’s root
  trace_root = path_join(db.root, stem);
  if (ERROR(trace_root == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  *root = trace_root;
  trace_root = NULL;

done:
  free(trace_root);

  return rc;
}
