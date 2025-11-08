#include "cbor.h"
#include "cmd_t.h"
#include "debug.h"
#include "input_t.h"
#include "list.h"
#include "output_t.h"
#include "trace_t.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xcache/trace.h>

int trace_save(const xc_trace_t trace, FILE *stream) {

  assert(stream != NULL);

  int rc = 0;

  // write the magic
  if (ERROR(fputs(CBOR_MAGIC, stream) < 0)) {
    rc = EIO;
    goto done;
  }

  // write the leading tag
  {
    const char tag[sizeof(uint64_t)] = {'\0', '\0', '\0', 'e',
                                        'c',  'a',  'r',  't'};
    uint64_t raw = 0;
    memcpy(&raw, tag, sizeof(raw));
    if (ERROR((rc = cbor_write_u64_raw(stream, raw, 0xc0))))
      goto done;
  }

  if (ERROR((rc = cmd_save(trace.cmd, stream))))
    goto done;

  assert(LIST_SIZE(&trace.inputs) <= UINT64_MAX);
  if (ERROR((rc = cbor_write_u64_raw(stream, (uint64_t)LIST_SIZE(&trace.inputs),
                                     0x80))))
    goto done;
  for (size_t i = 0; i < LIST_SIZE(&trace.inputs); ++i) {
    if (ERROR((rc = input_save(*LIST_AT(&trace.inputs, i), stream))))
      goto done;
  }

  assert(LIST_SIZE(&trace.outputs) <= UINT64_MAX);
  if (ERROR((rc = cbor_write_u64_raw(
                 stream, (uint64_t)LIST_SIZE(&trace.outputs), 0x80))))
    goto done;
  for (size_t i = 0; i < LIST_SIZE(&trace.outputs); ++i) {
    if (ERROR((rc = output_save(*LIST_AT(&trace.outputs, i), stream))))
      goto done;
  }

done:
  return rc;
}
