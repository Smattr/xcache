#include "debug.h"
#include "hash_t.h"
#include "input_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

int input_new(input_t *i, const char *path) {

  assert(i != NULL);
  assert(path != NULL);

  memset(i, 0, sizeof(*i));
  int rc = 0;

  i->path = strdup(path);
  if (ERROR(i->path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  {
    int r = hash_file(path, &i->digest);
    i->exists = r != ENOENT;
    i->accessible = r != EACCES && r != EPERM;
    i->is_directory = r == EISDIR;
    if (r != 0 && i->exists && i->accessible && !i->is_directory) {
      rc = r;
      goto done;
    }
  }

done:
  if (rc)
    input_free(*i);

  return rc;
}
