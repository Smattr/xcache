#include "cbor.h"
#include "debug.h"
#include "input_t.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

int input_save(const input_t input, FILE *stream) {

  assert(stream != NULL);

  int rc = 0;

  if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.tag))))
    goto done;

  if (ERROR((rc = cbor_write_str(stream, input.path))))
    goto done;

  if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.err))))
    goto done;

  switch (input.tag) {

  case INP_ACCESS:
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.access.flags))))
      goto done;
    break;

  case INP_READ:
    if (ERROR((rc = cbor_write_u64(stream, input.read.hash.data))))
      goto done;
    break;

  case INP_STAT:
    if (ERROR((rc = cbor_write_u64(stream, input.stat.is_lstat))))
      goto done;
    assert(sizeof(mode_t) <= sizeof(uint64_t));
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.stat.mode))))
      goto done;
    assert(sizeof(uid_t) <= sizeof(uint64_t));
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.stat.uid))))
      goto done;
    assert(sizeof(gid_t) <= sizeof(uint64_t));
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.stat.gid))))
      goto done;
    if (ERROR((rc = cbor_write_u64(stream, input.stat.size))))
      goto done;
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.stat.mtim.tv_sec))))
      goto done;
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.stat.mtim.tv_nsec))))
      goto done;
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.stat.ctim.tv_sec))))
      goto done;
    if (ERROR((rc = cbor_write_u64(stream, (uint64_t)input.stat.ctim.tv_nsec))))
      goto done;
    break;
  }

done:
  return rc;
}
