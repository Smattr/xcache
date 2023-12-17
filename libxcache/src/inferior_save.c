#include "cp.h"
#include "debug.h"
#include "output_t.h"
#include "path.h"
#include "tee_t.h"
#include "inferior_t.h"
#include "trace_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xcache/cmd.h>

static int append_stream(output_t *outputs, size_t *n_outputs,
                         const char *target, const char *copy,
                         const char *trace_root) {

  assert(outputs != NULL);
  assert(n_outputs != NULL);
  assert(target != NULL);
  assert(copy != NULL);
  assert(trace_root != NULL);
  assert(strncmp(copy, trace_root, strlen(trace_root)) == 0);
  assert(copy[strlen(trace_root)] == '/');

  output_t o = {0};
  int rc = 0;

  // how many bytes were written?
  struct stat st;
  if (ERROR(stat(copy, &st) < 0)) {
    rc = errno;
    goto done;
  }

  // if nothing was written, no need to save this output
  if (st.st_size == 0) {
    DEBUG("no bytes written to %s, so no need to save", target);
    (void)unlink(copy);
    goto done;
  }

  // construct an output for this
  o.tag = OUT_WRITE;
  o.path = strdup(target);
  if (ERROR(o.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  o.write.cached_copy = strdup(&copy[strlen(trace_root) + 1]);
  if (ERROR(o.write.cached_copy == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // append this output
  outputs[*n_outputs] = o;
  ++*n_outputs;
  o = (output_t){0};

done:
  output_free(o);

  return rc;
}

int inferior_save(inferior_t *inf, const xc_cmd_t cmd, const char *trace_root) {

  assert(trace_root != NULL);

  char *path = NULL;
  int fd = 0;
  FILE *f = NULL;
  output_t *outputs = NULL;
  size_t n_outputs = 0;
  int rc = 0;

  // drain stdout and stderr
  if (ERROR((rc = tee_join(inf->t_out))))
    goto done;
  if (ERROR((rc = tee_join(inf->t_err))))
    goto done;

  // account for the outputs we saw + stdout and stderr
  outputs = calloc(inf->n_outputs + 2, sizeof(outputs[0]));
  if (ERROR(outputs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // determine whether stdout and stderr need to be saved
  if (ERROR((rc = append_stream(outputs, &n_outputs, "/dev/stdout",
                                inf->t_out->copy_path, trace_root))))
    goto done;
  if (ERROR((rc = append_stream(outputs, &n_outputs, "/dev/stderr",
                                inf->t_err->copy_path, trace_root))))
    goto done;

  // finalise our other outputs
  for (size_t i = 0; i < inf->n_outputs; ++i) {
    if (ERROR((rc = output_dup(&outputs[n_outputs], inf->outputs[i]))))
      goto done;
    ++n_outputs;

    // if this was a file write, finalise it now
    if (outputs[i].tag == OUT_WRITE) {
      assert(outputs[i].write.cached_copy == NULL &&
             "output already has saved copy prior to finalisation");

      int src = open(outputs[i].path, O_RDONLY | O_CLOEXEC);
      if (ERROR(src < 0)) {
        rc = errno;
        goto done;
      }

      int dst = -1;
      if (ERROR((rc = path_make(trace_root, NULL, &dst,
                                &outputs[i].write.cached_copy)))) {
        (void)close(src);
        goto done;
      }

      rc = cp(dst, src);
      (void)close(dst);
      (void)close(src);
      if (ERROR(rc))
        goto done;
    }
  }

  // create a new trace file to save to
  if (ERROR((rc = path_make(trace_root, ".trace", &fd, NULL))))
    goto done;

  f = fdopen(fd, "w");
  if (ERROR(f == NULL)) {
    rc = errno;
    goto done;
  }
  fd = 0;

  // construct a trace object to write out
  const xc_trace_t trace = {.cmd = cmd,
                            .inputs = inf->inputs,
                            .n_inputs = inf->n_inputs,
                            .outputs = outputs,
                            .n_outputs = n_outputs};

  if (ERROR((rc = trace_save(trace, f))))
    goto done;

done:
  for (size_t i = 0; i < n_outputs; ++i)
    output_free(outputs[i]);
  free(outputs);
  if (f != NULL)
    (void)fclose(f);
  if (fd > 0)
    (void)close(fd);
  free(path);

  return rc;
}
