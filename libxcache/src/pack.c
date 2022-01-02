#include "pack.h"
#include "debug.h"
#include "fs_set.h"
#include "hash.h"
#include "macros.h"
#include "proc.h"
#include "trace.h"
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

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

static int pack_bool(FILE *f, bool value) {
  return write_u8(f, value ? 0xf5 : 0xf4);
}

static int pack_uint(FILE *f, uintmax_t value) {

  if (value <= 0x17)
    return write_u8(f, 0x00 + (uint8_t)value);

  if (value <= UINT8_MAX) {
    int r = write_u8(f, 0x18);
    if (ERROR(r != 0))
      return r;
    return write_u8(f, (uint8_t)value);
  }

  if (value <= UINT16_MAX) {
    int r = write_u8(f, 0x19);
    if (ERROR(r != 0))
      return r;
    return write_u16(f, (uint16_t)value);
  }

  if (value <= UINT32_MAX) {
    int r = write_u8(f, 0x1a);
    if (ERROR(r != 0))
      return r;
    return write_u32(f, (uint32_t)value);
  }

  assert(value <= UINT64_MAX);
  int r = write_u8(f, 0x1b);
  if (ERROR(r != 0))
    return r;
  return write_u64(f, (uint64_t)value);
}

static int pack_int(FILE *f, intmax_t value) {

  // pack this identically to unsigned, for simplicity
  uintmax_t u = 0;
  static_assert(sizeof(u) == sizeof(value));
  memcpy(&u, &value, sizeof(u));
  return pack_uint(f, u);
}

static int pack_string(FILE *f, const char *value) {

  if (value == NULL)
    return write_u8(f, 0xf6);

  size_t len = strlen(value);

  if (len <= 0x17) {
    int r = write_u8(f, 0x60 + (uint8_t)len);
    if (ERROR(r != 0))
      return r;

  } else if (len <= UINT8_MAX) {
    int r = write_u8(f, 0x78);
    if (ERROR(r != 0))
      return r;
    r = write_u8(f, (uint8_t)len);
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

/// byte used as a tag to indicate a trace entry
static const uint8_t TRACE_TAG = 'T';

int pack_trace(FILE *f, const xc_trace_t *trace) {

  assert(f != NULL);
  assert(trace != NULL);

  // write the tag
  {
    int r = pack_tag(f, TRACE_TAG);
    if (ERROR(r != 0))
      return r;
  }

  // serialise accessed files
  {
    int r = pack_array_length(f, trace->io.size);
    if (ERROR(r != 0))
      return r;
  }
  for (size_t i = 0; i < trace->io.size; ++i) {
    const fs_t *fs = &trace->io.base[i];
    int r = pack_string(f, fs->path);
    if (ERROR(r != 0))
      return r;
    r = pack_bool(f, fs->read);
    if (ERROR(r != 0))
      return r;
    r = pack_bool(f, fs->existed);
    if (ERROR(r != 0))
      return r;
    r = pack_bool(f, fs->accessible);
    if (ERROR(r != 0))
      return r;
    r = pack_bool(f, fs->is_directory);
    if (ERROR(r != 0))
      return r;
    r = pack_bool(f, fs->written);
    if (ERROR(r != 0))
      return r;
    r = pack_uint(f, fs->hash);
    if (ERROR(r != 0))
      return r;
    r = pack_string(f, fs->content_path);
    if (ERROR(r != 0))
      return r;
  }

  // serialise exit status
  {
    int r = pack_int(f, trace->exit_status);
    if (ERROR(r != 0))
      return r;
  }

  return 0;
}

static int read_u8(const void **base, size_t *size, uint8_t *value) {
  if (ERROR(*size < sizeof(*value)))
    return ENODATA;
  memcpy(value, *base, sizeof(*value));
  *base += sizeof(*value);
  *size -= sizeof(*value);
  return 0;
}

static int read_u16(const void **base, size_t *size, uint16_t *value) {
  if (ERROR(*size < sizeof(*value)))
    return ENODATA;
  uint16_t v = 0;
  memcpy(&v, *base, sizeof(v));
  *value = be16toh(v);
  *base += sizeof(*value);
  *size -= sizeof(*value);
  return 0;
}

static int read_u32(const void **base, size_t *size, uint32_t *value) {
  if (ERROR(*size < sizeof(*value)))
    return ENODATA;
  uint32_t v = 0;
  memcpy(&v, *base, sizeof(v));
  *value = be32toh(v);
  *base += sizeof(*value);
  *size -= sizeof(*value);
  return 0;
}

static int read_u64(const void **base, size_t *size, uint64_t *value) {
  if (ERROR(*size < sizeof(*value)))
    return ENODATA;
  uint64_t v = 0;
  memcpy(&v, *base, sizeof(v));
  *value = be64toh(v);
  *base += sizeof(*value);
  *size -= sizeof(*value);
  return 0;
}

static int unpack_array_length(const void **base, size_t *size,
                               size_t *length) {

  uint8_t byte = 0;
  {
    int r = read_u8(base, size, &byte);
    if (ERROR(r != 0))
      return r;
  }

  if (byte >= 0x80 && byte <= 0x80 + 0x17) {
    *length = byte - 0x80;
    return 0;
  }

  switch (byte) {

  case 0x98: {
    uint8_t l = 0;
    int r = read_u8(base, size, &l);
    if (ERROR(r != 0))
      return r;
    *length = l;
    break;
  }

  case 0x99: {
    uint16_t l = 0;
    int r = read_u16(base, size, &l);
    if (ERROR(r != 0))
      return r;
    *length = l;
    break;
  }

  case 0x9a: {
    uint32_t l = 0;
    int r = read_u32(base, size, &l);
    if (ERROR(r != 0))
      return r;
    *length = l;
    break;
  }

  case 0x9b: {
    uint64_t l = 0;
    int r = read_u64(base, size, &l);
    if (ERROR(r != 0))
      return r;
    *length = l;
    break;
  }

  default:
    return ENOMSG;
  }

  return 0;
}

static int unpack_string(const void **base, size_t *size, char **value) {

  uint8_t byte = 0;
  {
    int r = read_u8(base, size, &byte);
    if (ERROR(r != 0))
      return r;
  }

  size_t length = 0;

  switch (byte) {

  case 0xf6:
    *value = NULL;
    return 0;

  case 0x78: {
    uint8_t l = 0;
    int r = read_u8(base, size, &l);
    if (ERROR(r != 0))
      return r;
    length = l;
    break;
  }

  case 0x79: {
    uint16_t l = 0;
    int r = read_u16(base, size, &l);
    if (ERROR(r != 0))
      return r;
    length = l;
    break;
  }

  case 0x7a: {
    uint32_t l = 0;
    int r = read_u32(base, size, &l);
    if (ERROR(r != 0))
      return r;
    length = l;
    break;
  }

  case 0x7b: {
    uint64_t l = 0;
    int r = read_u64(base, size, &l);
    if (ERROR(r != 0))
      return r;
    length = l;
    break;
  }

  default:
    if (byte >= 0x60 && byte <= 0x60 + 0x17) {
      length = byte - 0x60;
      break;
    }
    return ENOMSG;
  }

  if (length > *size)
    return ENODATA;

  char *v = strndup(*base, length);
  if (ERROR(v == NULL))
    return ENOMEM;
  *base += length;
  *size -= length;

  *value = v;

  return 0;
}

static int unpack_bool(const void **base, size_t *size, bool *value) {

  uint8_t byte = 0;
  {
    int r = read_u8(base, size, &byte);
    if (ERROR(r != 0))
      return r;
  }

  if (byte == 0xf5) {
    *value = true;
  } else if (byte == 0xf4) {
    *value = false;
  } else {
    return ENOMSG;
  }

  return 0;
}

static int unpack_uint(const void **base, size_t *size, uintmax_t *value) {

  uint8_t byte = 0;
  {
    int r = read_u8(base, size, &byte);
    if (ERROR(r != 0))
      return r;
  }

  if (byte <= 0x00 + 0x17) {
    *value = byte;
    return 0;
  }

  switch (byte) {

  case 0x18: {
    uint8_t v = 0;
    int r = read_u8(base, size, &v);
    if (ERROR(r != 0))
      return r;
    *value = v;
    break;
  }

  case 0x19: {
    uint16_t v = 0;
    int r = read_u16(base, size, &v);
    if (ERROR(r != 0))
      return r;
    *value = v;
    break;
  }

  case 0x1a: {
    uint32_t v = 0;
    int r = read_u32(base, size, &v);
    if (ERROR(r != 0))
      return r;
    *value = v;
    break;
  }

  case 0x1b: {
    uint64_t v = 0;
    int r = read_u64(base, size, &v);
    if (ERROR(r != 0))
      return r;
    *value = v;
    break;
  }

  default:
    return ENOMSG;
  }

  return 0;
}

static int unpack_int(const void **base, size_t *size, intmax_t *value) {

  uintmax_t u = 0;
  {
    int r = unpack_uint(base, size, &u);
    if (ERROR(r != 0))
      return r;
  }
  static_assert(sizeof(u) == sizeof(*value));
  memcpy(value, &u, sizeof(*value));
  return 0;
}

static int unpack_tag(const void **base, size_t *size, uint8_t *tag) {
  return read_u8(base, size, tag);
}

int unpack_trace(const void *base, size_t size, xc_trace_t **trace) {

  assert(base != NULL && base != MAP_FAILED);
  assert(trace != NULL);

  // check the tag
  {
    uint8_t tag;
    int r = unpack_tag(&base, &size, &tag);
    if (ERROR(r != 0))
      return r;
    if (ERROR(tag != TRACE_TAG)) {
      DEBUG("read unexpected tag 0x%02x", (unsigned)tag);
      return ENOMSG;
    }
  }

  xc_trace_t *t = NULL;
  int rc = -1;

  t = calloc(1, sizeof(*t));
  if (ERROR(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // deserialise length of accessed files
  {
    size_t length = 0;
    rc = unpack_array_length(&base, &size, &length);
    if (ERROR(rc != 0))
      goto done;
    t->io.base = calloc(length, sizeof(t->io.base[0]));
    if (UNLIKELY(length > 0 && t->io.base == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    t->io.size = length;
    t->io.capacity = length;
  }

  // deserialise the accessed files themselves
  for (size_t i = 0; i < t->io.size; ++i) {
    fs_t *fs = &t->io.base[i];
    rc = unpack_string(&base, &size, &fs->path);
    if (ERROR(rc != 0))
      goto done;
    {
      bool v = false;
      rc = unpack_bool(&base, &size, &v);
      if (ERROR(rc != 0))
        goto done;
      fs->read = v;
    }
    {
      bool v = false;
      rc = unpack_bool(&base, &size, &v);
      if (ERROR(rc != 0))
        goto done;
      fs->existed = v;
    }
    {
      bool v = false;
      rc = unpack_bool(&base, &size, &v);
      if (ERROR(rc != 0))
        goto done;
      fs->accessible = v;
    }
    {
      bool v = false;
      rc = unpack_bool(&base, &size, &v);
      if (ERROR(rc != 0))
        goto done;
      fs->is_directory = v;
    }
    {
      bool v = false;
      rc = unpack_bool(&base, &size, &v);
      if (ERROR(rc != 0))
        goto done;
      fs->written = v;
    }
    {
      uintmax_t v = 0;
      rc = unpack_uint(&base, &size, &v);
      if (ERROR(rc != 0))
        goto done;
      if (v > HASH_MAX) {
        rc = ENOMSG;
        goto done;
      }
      fs->hash = (xc_hash_t)v;
    }
    rc = unpack_string(&base, &size, &fs->content_path);
    if (ERROR(rc != 0))
      goto done;
  }

  // deserialise exit status
  {
    intmax_t es = 0;
    rc = unpack_int(&base, &size, &es);
    if (ERROR(rc != 0))
      goto done;
    if (ERROR(es < INT_MIN || es > INT_MAX)) {
      rc = ERANGE;
      goto done;
    }
    t->exit_status = (int)es;
  }

  // success
  *trace = t;
  t = NULL;
  rc = 0;

done:
  if (UNLIKELY(t != NULL)) {
    for (size_t i = 0; i < t->io.size; ++i) {
      fs_t *fs = &t->io.base[i];
      free(fs->path);
      free(fs->content_path);
    }
    free(t->io.base);
  }
  free(t);

  return rc;
}
