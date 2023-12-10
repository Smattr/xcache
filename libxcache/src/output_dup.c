#include "debug.h"
#include "output_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

int output_dup(output_t *dst, const output_t src) {

  assert(dst != NULL);

  *dst = (output_t){0};
  output_t o = {0};
  int rc = 0;

  o.tag = src.tag;

  o.path = strdup(src.path);
  if (ERROR(o.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  switch (src.tag) {

  case OUT_CHMOD:
    o.chmod.mode = src.chmod.mode;
    break;

  case OUT_CHOWN:
    o.chown.uid = src.chown.uid;
    o.chown.gid = src.chown.gid;
    break;

  case OUT_MKDIR:
    o.mkdir.mode = src.mkdir.mode;
    break;

  case OUT_WRITE:
    // allow NULL to cope with an unfinalised output
    if (src.write.cached_copy != NULL) {
      o.write.cached_copy = strdup(src.write.cached_copy);
      if (ERROR(o.write.cached_copy == NULL)) {
        rc = ENOMEM;
        goto done;
      }
    }
    break;
  }

  *dst = o;
  o = (output_t){0};

done:
  output_free(o);

  return rc;
}
