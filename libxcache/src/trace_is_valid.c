#include "debug.h"
#include "trace.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <xcache/hash.h>
#include <xcache/trace.h>

bool xc_trace_is_valid(const xc_trace_t *trace) {

  if (ERROR(trace == NULL))
    return false;

  for (size_t i = 0; i < trace->io.size; ++i) {
    if (trace->io.base[i].read) {
      if (ERROR(trace->io.base[i].path == NULL))
        return false;
    }
  }

  // does the current hash of each file match those seen by the trace?
  for (size_t i = 0; i < trace->io.size; ++i) {

    if (!trace->io.base[i].read)
      continue;

    xc_hash_t hash;
    int rc = xc_hash_file(&hash, trace->io.base[i].path);
    bool exists = rc != ENOENT;
    bool accessible = rc != EACCES && rc != EPERM;
    bool is_directory = rc == EISDIR;
    if (ERROR(exists && accessible && !is_directory && rc != 0))
      return false;

    if (exists != trace->io.base[i].existed)
      return false;
    if (!exists)
      continue;

    if (accessible != trace->io.base[i].accessible)
      return false;
    if (!accessible)
      continue;

    if (is_directory != trace->io.base[i].is_directory)
      return false;
    if (is_directory)
      continue;

    if (hash != trace->io.base[i].hash)
      return false;
  }

  return true;
}
