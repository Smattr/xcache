#include "cbor.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>

int cbor_write_opt_str(FILE *stream, const char *value) {

  assert(stream != NULL);

  if (value == NULL) {
    if (ERROR(putc(0xf6, stream) < 0))
      return EIO;
    return 0;
  }

  return cbor_write_str(stream, value);
}
