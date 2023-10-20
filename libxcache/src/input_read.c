#include "cbor.h"
#include "input_t.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

int input_read(input_t *input, FILE *stream) {

  assert(input != NULL);
  assert(stream != NULL);

  *input = (input_t){0};
  input_t i = {0};
  int rc = 0;

  if ((rc = cbor_read_str(stream, &i.path)))
    goto done;

  {
    uint64_t stat_errno = 0;
    if ((rc = cbor_read_u64(stream, &stat_errno)))
      goto done;
    i.stat_errno = (int)stat_errno;
  }

  {
    uint64_t st_mode = 0;
    if ((rc = cbor_read_u64(stream, &st_mode)))
      goto done;
    i.st_mode = (mode_t)st_mode;
  }

  {
    uint64_t st_uid = 0;
    if ((rc = cbor_read_u64(stream, &st_uid)))
      goto done;
    i.st_uid = (uid_t)st_uid;
  }

  {
    uint64_t st_gid = 0;
    if ((rc = cbor_read_u64(stream, &st_gid)))
      goto done;
    i.st_gid = (gid_t)st_gid;
  }

  {
    uint64_t open_errno = 0;
    if ((rc = cbor_read_u64(stream, &open_errno)))
      goto done;
    i.open_errno = (int)open_errno;
  }

  if ((rc = hash_read(&i.digest, stream)))
    goto done;

  *input = i;
  i = (input_t){0};

done:
  input_free(i);

  return rc;
}
