/// @file
/// @brief Interpose on `sysconf`
///
/// Some tracees (e.g. GCC) call `sysconf` to probe characteristics of the
/// platform they are running on. For things like `_SC_PAGESIZE` and
/// `_SC_PHYS_PAGES` this unfortunately bottoms out on a syscall to `sysinfo`.
/// `sysinfo` is not something we can easily record and replay because some of
/// its fields will almost certainly change between runs (e.g. `loads`).
/// However these are fields that are not actually being consumed by the
/// `sysconf` caller. In other words, naïvely tracing this would incur a false
/// positive dependency.
///
/// To get out of this bind, we interpose on `sysconf`. We can then detect the
/// tightly scoped cases we know are safe and likely to be replayable, and
/// disable tracing during their call to avoid capturing the spurious
/// `sysinfo`.

#include "../../common/proccall.h"
#include "call.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <threads.h>
#include <unistd.h>

/// handle to libc’s `sysconf`
static long (*_Atomic real_sysconf)(int name);

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

  // if this is the first time we were called, interpose on `sysconf` now
  if (real_sysconf == NULL) {
    // we are potentially racing with other threads to set `real_sysconf`, but
    // the race is benign so just let it happen
    void *const sysconf_ptr = dlsym(RTLD_NEXT, "sysconf");
    if (sysconf_ptr == NULL) {
      errno = ENOSYS;
      return -1;
    }
    real_sysconf = sysconf_ptr;
  }

  // if we do not have a good understanding of this call, let it go through
  if (!known_sysconf(name))
    return real_sysconf(name);

  static thread_local size_t call_depth;

  // pass the name of the `sysconf` to our tracer
  call(CALL_SYSCONF, (void *)(intptr_t)name);

  // tell our tracer to ignore any syscalls that occur below
  if (call_depth == 0)
    call(CALL_OFF, NULL);
  ++call_depth;

  // make the actual call, that may transitively involve syscalls
  const long ret = real_sysconf(name);

  // tell our tracer to resume paying attention
  --call_depth;
  if (call_depth == 0)
    call(CALL_ON, NULL);

  return ret;
}
