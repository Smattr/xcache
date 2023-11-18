#include "cbor.h"
#include "debug.h"
#include <endian.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int cbor_read_u64_raw(FILE *stream, uint64_t *value, uint8_t bias) {

  const int tag_int = getc(stream);
  if (ERROR(tag_int < 0))
    return EIO;
  const uint8_t tag = (uint8_t)tag_int;

  if (0x0 + bias <= tag && tag <= 0x17 + bias) {
    *value = tag - bias;
    return 0;
  }

  if (tag == 0x18 + bias) {
    uint8_t v = 0;
    if (ERROR(fread(&v, sizeof(v), 1, stream) < 1))
      return EIO;
    *value = v;
    return 0;
  }

  if (tag == 0x19 + bias) {
    uint16_t v = 0;
    if (ERROR(fread(&v, sizeof(v), 1, stream) < 1))
      return EIO;
    *value = be16toh(v);
    return 0;
  }

  if (tag == 0x1a + bias) {
    uint32_t v = 0;
    if (ERROR(fread(&v, sizeof(v), 1, stream) < 1))
      return EIO;
    *value = be32toh(v);
    return 0;
  }

  if (tag == 0x1b + bias) {
    uint64_t v = 0;
    if (ERROR(fread(&v, sizeof(v), 1, stream) < 1))
      return EIO;
    *value = be64toh(v);
    return 0;
  }

  DEBUG("bad tag %" PRIu8, tag);
  return EIO;
}
