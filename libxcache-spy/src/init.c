#include <stdlib.h>

/// actions to perform before entering main
static __attribute__((constructor)) void init(void) {

  // TODO: signal an “ignore window”, otherwise we will have false positives
  // when _not_ using Glibc

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
  volatile char *ignored = malloc(128);
  if (ignored == NULL)
    abort();
  *ignored = 0;
  free((char *)ignored);
}
