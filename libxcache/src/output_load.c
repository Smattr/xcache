#include "cbor.h"
#include "debug.h"
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

  {
    uint64_t tag = 0;
    if (ERROR((rc = cbor_read_u64(stream, &tag))))
      goto done;
    o.tag = tag;
  }

  if (ERROR((rc = cbor_read_str(stream, &o.path))))
    goto done;

  switch (o.tag) {

  case OUT_CHMOD: {
    uint64_t mode = 0;
    if (ERROR((rc = cbor_read_u64(stream, &mode))))
      goto done;
    o.chmod.mode = (mode_t)mode;
  } break;

  case OUT_CHOWN: {
    uint64_t uid = 0;
    if (ERROR((rc = cbor_read_u64(stream, &uid))))
      goto done;
    o.chown.uid = (uid_t)uid;
  }

    {
      uint64_t gid = 0;
      if (ERROR((rc = cbor_read_u64(stream, &gid))))
        goto done;
      o.chown.gid = (gid_t)gid;
    }
    break;

  case OUT_MKDIR: {
    uint64_t mode = 0;
    if (ERROR((rc = cbor_read_u64(stream, &mode))))
      goto done;
    o.mkdir.mode = (mode_t)mode;
  } break;

  case OUT_WRITE:
    if (ERROR((rc = cbor_read_opt_str(stream, &o.write.cached_copy))))
      goto done;
    break;

  default:
    DEBUG("invalid output tag %d\n", (int)o.tag);
    rc = EPROTO;
    goto done;
  }

  *output = o;
  o = (output_t){0};

done:
  output_free(o);

  return rc;
}
