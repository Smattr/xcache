#include "db_t.h"
#include <stdlib.h>
#include <unistd.h>
#include <xcache/db.h>

void xc_db_close(xc_db_t *db) {

  if (db == NULL)
    return;

  if (db->root > 0)
    (void)close(db->root);

  free(db);
}
