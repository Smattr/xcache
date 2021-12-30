#include "pack.h"
#include "debug.h"
#include "proc.h"
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xcache/proc.h>

// these serialisation functions could use any arbitrary encoding as long as the
// decoding side matched, but for convenience we use CBOR

static int write_u8(FILE *f, uint8_t value) {
  if (ERROR(fwrite(&value, sizeof(value), 1, f) != 1))
    return EIO;
  return 0;
}

static int write_u16(FILE *f, uint16_t value) {
  value = htobe16(value);
  if (ERROR(fwrite(&value, sizeof(value), 1, f) != 1))
    return EIO;
  return 0;
}

static int write_u32(FILE *f, uint32_t value) {
  value = htobe32(value);
  if (ERROR(fwrite(&value, sizeof(value), 1, f) != 1))
    return EIO;
  return 0;
}

static int write_u64(FILE *f, uint64_t value) {
  value = htobe64(value);
  if (ERROR(fwrite(&value, sizeof(value), 1, f) != 1))
    return EIO;
  return 0;
}

static int pack_array_length(FILE *f, size_t value) {

  if (value <= 0x17)
    return write_u8(f, 0x80 + (uint8_t)value);

  if (value <= UINT8_MAX) {
    int r = write_u8(f, 0x98);
    if (ERROR(r != 0))
      return r;
    return write_u8(f, (uint8_t)value);
  }

  if (value <= UINT16_MAX) {
    int r = write_u8(f, 0x99);
    if (ERROR(r != 0))
      return r;
    return write_u16(f, (uint16_t)value);
  }

  if (value <= UINT32_MAX) {
    int r = write_u8(f, 0x9a);
    if (ERROR(r != 0))
      return r;
    return write_u32(f, (uint32_t)value);
  }

  assert(value <= UINT64_MAX);
  int r = write_u8(f, 0x9b);
  if (ERROR(r != 0))
    return r;
  return write_u64(f, (uint64_t)value);
}

static int pack_string(FILE *f, const char *value) {

  size_t len = strlen(value);

  if (len <= 0x17) {
    int r = write_u8(f, 0x60 + (uint8_t)len);
    if (ERROR(r != 0))
      return r;

  } else if (len <= UINT8_MAX) {
    int r = write_u8(f, 0x78);
    if (ERROR(r != 0))
      return r;
    r = write_u16(f, (uint16_t)len);
    if (ERROR(r != 0))
      return r;

  } else if (len <= UINT16_MAX) {
    int r = write_u8(f, 0x79);
    if (ERROR(r != 0))
      return r;
    r = write_u16(f, (uint16_t)len);
    if (ERROR(r != 0))
      return r;

  } else if (len <= UINT32_MAX) {
    int r = write_u8(f, 0x7a);
    if (ERROR(r != 0))
      return r;
    r = write_u32(f, (uint32_t)len);
    if (ERROR(r != 0))
      return r;

  } else {
    assert(len <= UINT64_MAX);
    int r = write_u8(f, 0x7b);
    if (ERROR(r != 0))
      return r;
    r = write_u64(f, (uint64_t)len);
    if (ERROR(r != 0))
      return r;
  }

  if (ERROR(fwrite(value, 1, len, f) != len))
    return EIO;

  return 0;
}

static int pack_tag(FILE *f, uint8_t tag) { return write_u8(f, tag); }

/// byte used as a tag to indicate a proc entry
static const uint8_t PACK_TAG = 'P';

int pack_proc(FILE *f, const xc_proc_t *proc) {

  assert(f != NULL);
  assert(proc != NULL);

  // write the tag
  {
    int r = pack_tag(f, PACK_TAG);
    if (ERROR(r != 0))
      return r;
  }

  // serialise argv
  {
    int r = pack_array_length(f, proc->argc);
    if (ERROR(r != 0))
      return r;
  }
  for (size_t i = 0; i < proc->argc; ++i) {
    int r = pack_string(f, proc->argv[i]);
    if (ERROR(r != -0))
      return r;
  }

  // serialise cwd
  {
    int r = pack_string(f, proc->cwd);
    if (ERROR(r != 0))
      return r;
  }

  return 0;
}
