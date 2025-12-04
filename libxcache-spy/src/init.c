#include "../../common/proccall.h"
#include "call.h"
#include "version.h"
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/// actions to perform before entering main
static __attribute__((constructor)) void init(void) {

  // alert the tracer to our presence
  call(CALL_HELLO, xc_spy_version());

  // tell our tracer to ignore any syscalls that occur below
  call(CALL_OFF, NULL);

  // Glibc’s allocator implements a thread-local cache it calls “tcache”. During
  // its initialisation, it calls `getrandom`, a function Xcache would usually
  // consider uncacheable. We want to ignore this `getrandom` call in order to
  // retain the ability to trace programs that use the heap, but _not_ ignore
  // other calls to `getrandom`. But we do not know whether the tracee uses the
  // heap and thus whether it will trigger this initialisation. Simply ignoring
  // the first `getrandom` we see results in programs that call `getrandom` but
  // do not use the heap incorrectly being considered cacheable.
  //
  // To avoid this ambiguity, force heap usage immediately. This means the first
  // `getrandom` call will always be from tcache initialisation (if we are using
  // Glibc).
  {
    volatile char *ignored = malloc(128);
    if (ignored == NULL)
      abort();
    *ignored = 0;
    free((char *)ignored);
  }

  // `mktemp` and its cousins call `getrandom` to seed themselves. We want to
  // avoid seeing these (unreplayable) `getrandom` calls that are inessential to
  // reproducing behaviour. So make a spurious `mktemp` call to force seeding
  // now.
  {
    const char *tmp = getenv("TMPDIR");
    if (tmp == NULL)
      tmp = "/tmp";
    const int required = snprintf(NULL, 0, "%s/probe.XXXXXX", tmp);
    assert(required > 0);
    char *const buffer = malloc((size_t)required + 1);
    if (buffer == NULL) {
      fputs("libxcache-spy: out of memory\n", stderr);
      abort();
    }
    (void)snprintf(buffer, (size_t)required + 1, "%s/probe.XXXXXX", tmp);
    const int fd = mkostemp(buffer, O_CLOEXEC);
    if (fd < 0) {
      fprintf(stderr, "libxcache-spy: failed to create temporary file: %s\n",
              strerror(errno));
      abort();
    }
    (void)close(fd);
    (void)unlink(buffer);
    free(buffer);
  }

  // tell our tracer to resume paying attention
  call(CALL_ON, NULL);
}
