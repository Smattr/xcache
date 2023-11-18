#include "cbor.h"
#include "output_t.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int output_load(output_t *output, FILE *stream) {

  assert(output != NULL);
  assert(stream != NULL);

  *output = (output_t){0};
  output_t o = {0};
  int rc = 0;

  if ((rc = cbor_read_str(stream, &o.path)))
    goto done;

  {
    uint64_t st_mode = 0;
    if ((rc = cbor_read_u64(stream, &st_mode)))
      goto done;
    o.st_mode = (mode_t)st_mode;
  }

  {
    uint64_t st_uid = 0;
    if ((rc = cbor_read_u64(stream, &st_uid)))
      goto done;
    o.st_uid = (uid_t)st_uid;
  }

  {
    uint64_t st_gid = 0;
    if ((rc = cbor_read_u64(stream, &st_gid)))
      goto done;
    o.st_gid = (gid_t)st_gid;
  }

  if ((rc = cbor_read_str(stream, &o.cached_copy)))
    goto done;

  *output = o;
  o = (output_t){0};

done:
  output_free(o);

  return rc;
}
