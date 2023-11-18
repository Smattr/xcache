#include "cbor.h"
#include <stdint.h>
#include <stdio.h>

int cbor_write_u64(FILE *stream, uint64_t value) {
  return cbor_write_u64_raw(stream, value, 0);
}
