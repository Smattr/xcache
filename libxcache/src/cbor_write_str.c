#include "cbor.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int cbor_write_str(FILE *stream, const char *value) {

  assert(stream != NULL);
  assert(value != NULL);

  const size_t len = strlen(value);
  if (ERROR(len > UINT64_MAX))
    return ERANGE;

  int rc = 0;

  if (ERROR((rc = cbor_write_u64_raw(stream, (uint64_t)len, 0x60))))
    goto done;
  if (ERROR(fwrite(value, len, 1, stream) < 1)) {
    rc = EIO;
    goto done;
  }

done:
  return rc;
}
