/// @file
/// @brief Interpose on `mktemp` and friends
///
/// `mktemp` and its cousins optionally call `getrandom` to seed themselves. We
/// want to avoid seeing these (unreplayable) `getrandom` calls that are
/// inessential to reproducing behaviour. So ignore `getrandom` while calling
/// these functions.

#include "call.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>

/// `mktemp` wrapper
char *mktemp(char *template) {

  // handle to libc’s `mktemp`
  static char *(*_Atomic real)(char *template);

  // if this is the first time we were called, interpose on `mktemp` now
  if (real == NULL) {
    // we are potentially racing with other threads to set `real`, but the race
    // is benign so just let it happen
    void *const ptr = dlsym(RTLD_NEXT, "mktemp");
    if (ptr == NULL) {
      errno = ENOSYS;
      return NULL;
    }
    real = ptr;
  }

  // disable `getrandom` tracing
  call_rng_off();

  char *const ret = real(template);

  // re-enable `getrandom` tracing
  call_rng_on();

  return ret;
}

/// `mkstemp` wrapper
int mkstemp(char *template) {

  // handle to libc’s `mkstemp`
  static int (*_Atomic real)(char *template);

  // if this is the first time we were called, interpose on `mkstemp` now
  if (real == NULL) {
    // we are potentially racing with other threads to set `real`, but the race
    // is benign so just let it happen
    void *const ptr = dlsym(RTLD_NEXT, "mkstemp");
    if (ptr == NULL) {
      errno = ENOSYS;
      return -1;
    }
    real = ptr;
  }

  // disable `getrandom` tracing
  call_rng_off();

  const int ret = real(template);

  // re-enable `getrandom` tracing
  call_rng_on();

  return ret;
}

/// `mkostemp` wrapper
int mkostemp(char *template, int flags) {

  // handle to libc’s `mkostemp`
  static int (*_Atomic real)(char *template, int flags);

  // if this is the first time we were called, interpose on `mkostemp` now
  if (real == NULL) {
    // we are potentially racing with other threads to set `real`, but the race
    // is benign so just let it happen
    void *const ptr = dlsym(RTLD_NEXT, "mkostemp");
    if (ptr == NULL) {
      errno = ENOSYS;
      return -1;
    }
    real = ptr;
  }

  // disable `getrandom` tracing
  call_rng_off();

  const int ret = real(template, flags);

  // re-enable `getrandom` tracing
  call_rng_on();

  return ret;
}

/// `mkstemps` wrapper
int mkstemps(char *template, int suffixlen) {

  // handle to libc’s `mkstemps`
  static int (*_Atomic real)(char *template, int suffixlen);

  // if this is the first time we were called, interpose on `mkstemps` now
  if (real == NULL) {
    // we are potentially racing with other threads to set `real`, but the race
    // is benign so just let it happen
    void *const ptr = dlsym(RTLD_NEXT, "mkstemps");
    if (ptr == NULL) {
      errno = ENOSYS;
      return -1;
    }
    real = ptr;
  }

  // disable `getrandom` tracing
  call_rng_off();

  const int ret = real(template, suffixlen);

  // re-enable `getrandom` tracing
  call_rng_on();

  return ret;
}

/// `mkostemps` wrapper
int mkostemps(char *template, int suffixlen, int flags) {

  // handle to libc’s `mkostemps`
  static int (*_Atomic real)(char *template, int suffixlen, int flags);

  // if this is the first time we were called, interpose on `mkostemps` now
  if (real == NULL) {
    // we are potentially racing with other threads to set `real`, but the race
    // is benign so just let it happen
    void *const ptr = dlsym(RTLD_NEXT, "mkostemps");
    if (ptr == NULL) {
      errno = ENOSYS;
      return -1;
    }
    real = ptr;
  }

  // disable `getrandom` tracing
  call_rng_off();

  const int ret = real(template, suffixlen, flags);

  // re-enable `getrandom` tracing
  call_rng_on();

  return ret;
}
