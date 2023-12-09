#include "cbor.h"
#include "debug.h"
#include "output_t.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int output_save(const output_t output, FILE *stream) {

  assert(stream != NULL);

  int rc = 0;

  if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.tag))))
    goto done;

  if (ERROR((rc = cbor_write_str(stream, output.path))))
    goto done;

  switch (output.tag) {
  case OUT_CHMOD:
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.chmod.mode))))
      goto done;
    break;

  case OUT_CHOWN:
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.chown.uid))))
      goto done;

    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.chown.gid))))
      goto done;
    break;

  case OUT_MKDIR:
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.mkdir.mode))))
      goto done;
    break;

  case OUT_WRITE:
    if (ERROR((rc = cbor_write_opt_str(stream, output.write.cached_copy))))
      goto done;
    break;
  }

done:
  return rc;
}
