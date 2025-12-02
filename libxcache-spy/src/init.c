#include "../../common/proccall.h"
#include "call.h"
#include "version.h"
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/// is recording of our actions currently suppressed?
static bool tracing_disabled;

/// handle to libc’s `sysconf`
static long (*real_sysconf)(int name);

/// handle to libc’s `getenv`
static char *(*real_getenv)(const char *name);

/// actions to perform before entering main
static __attribute__((constructor)) void init(void) {

  // alert the tracer to our presence
  call(CALL_HELLO, xc_spy_version());

  // tell our tracer to ignore any syscalls that occur below
  call(CALL_OFF, NULL);
  tracing_disabled = true;

  // interpose on environment functions
  assert(real_getenv == NULL);
  real_getenv = dlsym(RTLD_NEXT, "getenv");
  if (real_getenv == NULL) {
    fprintf(stderr, "libxcache-spy: failed to locate `getenv` symbol: %s\n",
            dlerror());
    abort();
  }

  // Some tracees (e.g. GCC) call `sysconf` to probe characteristics of the
  // platform they are running on. For things like `_SC_PAGESIZE` and
  // `_SC_PHYS_PAGES` this unfortunately bottoms out on a syscall to `sysinfo`.
  // `sysinfo` is not something we can easily record and replay because some of
  // its fields will almost certainly change between runs (e.g. `loads`).
  // However these are fields that are not actually being consumed by the
  // `sysconf` caller. In other words, naïvely tracing this would incur a false
  // positive dependency.
  //
  // To get out of this bind, interpose on `sysconf`. We can then detect the
  // tightly scoped cases we know are safe and likely to be replayable, and
  // disable tracing during their call to avoid capturing the spurious
  // `sysinfo`.
  assert(real_sysconf == NULL);
  real_sysconf = dlsym(RTLD_NEXT, "sysconf");
  if (real_sysconf == NULL) {
    fprintf(stderr, "libxcache-spy: failed to locate `sysconf` symbol: %s\n",
            dlerror());
    abort();
  }

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
  tracing_disabled = false;
  call(CALL_ON, NULL);
}

/// is this a `sysconf` name whose behaviour we have vetted for replayability?
static bool known_sysconf(int name) {
  if (name == _SC_PAGESIZE)
    return true;
  if (name == _SC_PHYS_PAGES)
    return true;
  return false;
}

/// `sysconf` wrapper
long sysconf(int name) {

  // if tracing is already disabled, just let this call go through
  if (tracing_disabled)
    return real_sysconf(name);

  // if we do not have a good understanding of this call, let it go through
  if (!known_sysconf(name))
    return real_sysconf(name);

  // pass the name of the `sysconf` to our tracer
  call(CALL_SYSCONF, (void *)(intptr_t)name);

  // tell our tracer to ignore any syscalls that occur below
  call(CALL_OFF, NULL);
  tracing_disabled = true;

  // make the actual call, that may transitively involve syscalls
  const long ret = real_sysconf(name);

  // tell our tracer to resume paying attention
  tracing_disabled = false;
  call(CALL_ON, NULL);

  return ret;
}

/// `getenv` wrapper
char *getenv(const char *name) {

  // tell our tracer about this call
  call(CALL_GETENV, name);

  return real_getenv(name);
}
