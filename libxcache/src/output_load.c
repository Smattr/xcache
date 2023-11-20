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
    uint64_t mode = 0;
    if ((rc = cbor_read_u64(stream, &mode)))
      goto done;
    o.mode = (mode_t)mode;
  }

  {
    uint64_t uid = 0;
    if ((rc = cbor_read_u64(stream, &uid)))
      goto done;
    o.uid = (uid_t)uid;
  }

  {
    uint64_t gid = 0;
    if ((rc = cbor_read_u64(stream, &gid)))
      goto done;
    o.gid = (gid_t)gid;
  }

  if ((rc = cbor_read_str(stream, &o.cached_copy)))
    goto done;

  *output = o;
  o = (output_t){0};

done:
  output_free(o);

  return rc;
}
