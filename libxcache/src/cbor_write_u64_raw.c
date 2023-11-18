#include "cbor.h"
#include "debug.h"
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int cbor_write_u64_raw(FILE *stream, uint64_t value, uint8_t bias) {

  assert(stream != NULL);

  if (value <= 0x17) {
    const uint8_t tag = (uint8_t)value + bias;
    if (ERROR(putc(tag, stream) < 0))
      return EIO;
    return 0;
  }

  if (value <= UINT8_MAX) {
    const uint8_t tag = 0x18 + bias;
    if (ERROR(putc(tag, stream) < 0))
      return EIO;
    if (ERROR(putc((int)value, stream) < 0))
      return EIO;
    return 0;
  }

  if (value <= UINT16_MAX) {
    const uint8_t tag = 0x19 + bias;
    if (ERROR(putc(tag, stream) < 0))
      return EIO;
    const uint16_t payload = htobe16((uint16_t)value);
    if (ERROR(fwrite(&payload, sizeof(payload), 1, stream) < 1))
      return EIO;
    return 0;
  }

  if (value <= UINT32_MAX) {
    const uint8_t tag = 0x1a + bias;
    if (ERROR(putc(tag, stream) < 0))
      return EIO;
    const uint32_t payload = htobe32((uint32_t)value);
    if (ERROR(fwrite(&payload, sizeof(payload), 1, stream) != 1))
      return EIO;
    return 0;
  }

  const uint8_t tag = 0x1b + bias;
  if (ERROR(putc(tag, stream) < 0))
    return EIO;
  const uint64_t payload = htobe64(value);
  if (ERROR(fwrite(&payload, sizeof(payload), 1, stream) != 1))
    return EIO;

  return 0;
}
