#include "db.h"
#include "debug.h"
#include "hash.h"
#include "pack.h"
#include "path.h"
#include "trace.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcache/db.h>
#include <xcache/hash.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

int xc_db_load(const xc_db_t *db, const xc_proc_t *question,
               xc_trace_t **answer) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(question == NULL))
    return EINVAL;

  if (ERROR(answer == NULL))
    return EINVAL;

  char *input = NULL;
  int in = -1;
  void *in_base = MAP_FAILED;
  size_t in_size = 0;
  char *buffer_base = NULL;
  FILE *buffer = NULL;
  xc_trace_t *t = NULL;
  xc_trace_t *globalised = NULL;
  int rc = -1;

  // figure out from where to load this trace
  xc_hash_t hash = 0;
  rc = xc_hash_proc(&hash, question);
  if (ERROR(rc != 0))
    goto done;
  char hash_str[sizeof(hash) * 2 + 1];
  snprintf(hash_str, sizeof(hash_str), "%" PRIxHASH, hash);
  input = path_join(db->root, hash_str);
  if (ERROR(input == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // open and mmap it so we can inspect its contents
  in = open(input, O_RDONLY | O_CLOEXEC);
  if (ERROR(in < 0)) {
    rc = errno;
    goto done;
  }
  struct stat st;
  if (ERROR(fstat(in, &st) < 0)) {
    rc = errno;
    goto done;
  }
  in_size = st.st_size;
  in_base = mmap(NULL, in_size, PROT_READ, MAP_PRIVATE, in, 0);
  if (ERROR(in_base == MAP_FAILED)) {
    rc = errno;
    goto done;
  }

  // serialise the query
  size_t buffer_size = 0;
  buffer = open_memstream(&buffer_base, &buffer_size);
  if (ERROR(buffer == NULL)) {
    rc = errno;
    goto done;
  }
  rc = pack_proc(buffer, question);
  if (ERROR(rc != 0))
    goto done;
  {
    int r = fclose(buffer);
    buffer = NULL;
    if (ERROR(r == EOF)) {
      rc = errno;
      goto done;
    }
  }

  // does the serialised query in the cache entry match?
  size_t proc_size = buffer_size - 1 /* for NUL */;
  if (in_size < proc_size || memcmp(in_base, buffer_base, proc_size) != 0) {
    // hash collision
    rc = ENOENT;
    goto done;
  }

  // deserialise the trace that follows in the file
  rc = unpack_trace(in_base + proc_size, in_size - proc_size, &t);
  if (ERROR(rc != 0))
    goto done;

  // globalise the trace, so it can be replayed
  rc = trace_globalise(db, &globalised, t);
  if (ERROR(rc != 0))
    goto done;

  // success
  *answer = globalised;
  globalised = NULL;
  rc = 0;

done:
  if (globalised != NULL)
    trace_deinit(globalised);
  free(globalised);
  if (t != NULL)
    trace_deinit(t);
  free(t);
  if (buffer != NULL)
    (void)fclose(buffer);
  if (buffer_base != NULL)
    free(buffer_base);
  if (in_base != MAP_FAILED)
    (void)munmap(in_base, in_size);
  if (in >= 0)
    (void)close(in);
  free(input);

  return rc;
}
