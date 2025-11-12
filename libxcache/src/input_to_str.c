#include "input_t.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

char *input_to_str(const input_t input) {

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  char *ret = NULL;

  stream = open_memstream(&buffer, &buffer_size);
  if (stream == NULL)
    goto done;

  if (fputs("input_t{", stream) < 0)
    goto done;

  switch (input.tag) {

  case INP_ACCESS:
    if (fputs(".tag = INP_ACCESS", stream) < 0)
      goto done;
    break;

  case INP_READ:
    if (fputs(".tag = INP_READ", stream) < 0)
      goto done;
    break;

  case INP_READLINK:
    if (fputs(".tag = INP_READLINK", stream) < 0)
      goto done;
    break;

  case INP_STAT:
    if (fputs(".tag = INP_STAT", stream) < 0)
      goto done;
    break;

  default:
    if (fprintf(stream, ".tag = %d (unknown)", (int)input.tag) < 0)
      goto done;
    break;
  }

  if (fprintf(stream, ", .path = \"%s\"", input.path) < 0)
    goto done;
  if (fprintf(stream, ", .err = %d", input.err) < 0)
    goto done;

  switch (input.tag) {

  case INP_ACCESS:
    if (fprintf(stream, ", .access.flags = %d", input.access.flags) < 0)
      goto done;
    break;

  case INP_READ:
    if (fprintf(stream, ", .read.hash = 0x%" PRIx64, input.read.hash.data) < 0)
      goto done;
    break;

  case INP_READLINK:
    if (fprintf(stream, ", .readlink.hash = 0x%" PRIx64,
                input.readlink.hash.data) < 0)
      goto done;
    break;

  case INP_STAT:
    if (fprintf(stream, ", .stat.is_lstat = %s",
                input.stat.is_lstat ? "true" : "false") < 0)
      goto done;
    if (fprintf(stream, ", .stat.mode = %ld", (long)input.stat.mode) < 0)
      goto done;
    if (fprintf(stream, ", .stat.mode = %ld", (long)input.stat.mode) < 0)
      goto done;
    if (fprintf(stream, ", .stat.uid = %ld", (long)input.stat.uid) < 0)
      goto done;
    if (fprintf(stream, ", .stat.gid = %ld", (long)input.stat.gid) < 0)
      goto done;
    // TODO timespec
    break;
  }

  if (fputs("}", stream) < 0)
    goto done;

  fclose(stream);
  stream = NULL;
  ret = buffer;
  buffer = NULL;

done:
  if (stream != NULL)
    (void)fclose(stream);
  free(buffer);

  return ret;
}
