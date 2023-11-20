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

  if (ERROR((rc = cbor_write_str(stream, output.path))))
    goto done;

  if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.mode))))
    goto done;

  if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.uid))))
    goto done;

  if (ERROR((rc = cbor_write_u64(stream, (uint64_t)output.gid))))
    goto done;

  if (ERROR((rc = cbor_write_opt_str(stream, output.cached_copy))))
    goto done;

done:
  return rc;
}
