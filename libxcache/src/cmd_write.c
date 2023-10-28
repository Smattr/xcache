#include "cbor.h"
#include "cmd_t.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xcache/cmd.h>

int cmd_write(const xc_cmd_t cmd, FILE *stream) {

  assert(stream != NULL);

  if (ERROR(cmd.argc > UINT64_MAX))
    return ERANGE;

  int rc = 0;

  if (ERROR((rc = cbor_write_u64_raw(stream, (uint64_t)cmd.argc, 0x80))))
    goto done;
  for (size_t i = 0; i < cmd.argc; ++i) {
    if (ERROR((rc = cbor_write_str(stream, cmd.argv[i]))))
      goto done;
  }

  if (ERROR((rc = cbor_write_str(stream, cmd.cwd))))
    goto done;

done:
  return rc;
}
