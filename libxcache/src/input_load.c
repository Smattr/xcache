#include "cbor.h"
#include "debug.h"
#include "input.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

int input_load(input_t *input, FILE *stream) {

  assert(input != NULL);
  assert(stream != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  {
    uint64_t tag = 0;
    if (ERROR((rc = cbor_read_u64(stream, &tag))))
      goto done;
    i.tag = tag;
  }

  if (ERROR((rc = cbor_read_str(stream, &i.path))))
    goto done;

  {
    uint64_t err = 0;
    if (ERROR((rc = cbor_read_u64(stream, &err))))
      goto done;
    i.err = (int)err;
  }

  switch (i.tag) {

    // use `goto ok` in this switch instead of `break` so we can retain the
    // compiler warning benefits of -Wswitch while also catching malformed tags
#define break break not allowed in this switch

  case INP_ACCESS: {
    uint64_t mode = 0;
    if (ERROR((rc = cbor_read_u64(stream, &mode))))
      goto done;
    i.access.mode = (int)mode;
  }
    {
      uint64_t flags = 0;
      if (ERROR((rc = cbor_read_u64(stream, &flags))))
        goto done;
      i.access.flags = (int)flags;
    }
    goto ok;

  case INP_READ:
    if (ERROR((rc = cbor_read_u64(stream, &i.read.hash.data))))
      goto done;
    goto ok;

  case INP_STAT: {
    uint64_t is_lstat = 0;
    if (ERROR((rc = cbor_read_u64(stream, &is_lstat))))
      goto done;
    if (ERROR(is_lstat != 0 && is_lstat != 1)) {
      rc = EPROTO;
      goto done;
    }
    i.stat.is_lstat = (bool)is_lstat;
  }
    {
      uint64_t mode = 0;
      if (ERROR((rc = cbor_read_u64(stream, &mode))))
        goto done;
      i.stat.mode = (mode_t)mode;
    }
    {
      uint64_t uid = 0;
      if (ERROR((rc = cbor_read_u64(stream, &uid))))
        goto done;
      i.stat.uid = (uid_t)uid;
    }
    {
      uint64_t gid = 0;
      if (ERROR((rc = cbor_read_u64(stream, &gid))))
        goto done;
      i.stat.gid = (gid_t)gid;
    }
    {
      uint64_t size = 0;
      if (ERROR((rc = cbor_read_u64(stream, &size))))
        goto done;
      i.stat.size = (size_t)size;
    }
    {
      uint64_t tv_sec = 0;
      if (ERROR((rc = cbor_read_u64(stream, &tv_sec))))
        goto done;
      i.stat.mtim.tv_sec = (time_t)tv_sec;
    }
    {
      uint64_t tv_nsec = 0;
      if (ERROR((rc = cbor_read_u64(stream, &tv_nsec))))
        goto done;
      i.stat.mtim.tv_nsec = (long)tv_nsec;
    }
    {
      uint64_t tv_sec = 0;
      if (ERROR((rc = cbor_read_u64(stream, &tv_sec))))
        goto done;
      i.stat.ctim.tv_sec = (time_t)tv_sec;
    }
    {
      uint64_t tv_nsec = 0;
      if (ERROR((rc = cbor_read_u64(stream, &tv_nsec))))
        goto done;
      i.stat.ctim.tv_nsec = (long)tv_nsec;
    }
    goto ok;

  case INP_SYSCONF: {
    uint64_t name = 0;
    if (ERROR((rc = cbor_read_u64(stream, &name))))
      goto done;
    i.sysconf.name = (int)name;
  }
    {
      uint64_t ret = 0;
      if (ERROR((rc = cbor_read_u64(stream, &ret))))
        goto done;
      i.sysconf.ret = (long)ret;
    }
    goto ok;

  case INP_GETENV:
    if (ERROR((rc = cbor_read_str(stream, &i.getenv.key))))
      goto done;
    if (ERROR((rc = cbor_read_opt_str(stream, &i.getenv.value))))
      goto done;
    goto ok;

  case INP_UNLINK_PRE:
    // nothing to be done
    goto ok;

#undef break
  }

  DEBUG("invalid input tag %d", (int)i.tag);
  rc = EPROTO;
  goto done;

ok:
  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
