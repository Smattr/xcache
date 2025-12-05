/// @file
/// @brief interface for IPC between tracer and subprocess
///
/// The subprocess (tracee) needs to communicate certain out-of-band information
/// to the tracer. That is, actions it takes that need to be recorded but do not
/// result in syscalls that the tracer can see. An intuitive example is `getenv`
/// calls. To do this, libxcache injects an in-process monitoring component
/// (libxcache-spy) into the subprocess.
///
/// libxcache-spy needs a way of communicating back to the tracer. A pipe, like
/// the ones used to capture stdout and stderr, is the obvious solution. But
/// this results in a race condition. Some actions libxcache-spy wants to tell
/// the tracer about (e.g. “ignore the next few syscalls”) will race with
/// syscalls themselves if communicated via a write through a pipe. To avoid
/// this, communication is done via ioctls on an invalid file descriptor,
/// forcing it to be synchronous and reliably ordered with respect to other
/// syscalls. The ioctls themselves have no effect in the tracee and result in
/// an `EBADF`.

#pragma once

#include <errno.h>
#include <stdint.h>

/// file descriptor subprocess uses to message the tracer
///
/// This can be any arbitrary value that will never be a valid open file
/// descriptor or `AT_FDCWD`. linux/fcntl.h claims:
///
///   Reserved kernel ranges [-100], [-10000, -40000].
///
/// and uses the value `-10000 - EBADF` for its `FD_INVALID`. Reportedly some
/// userspace software uses `-EBADF` in `*at` syscalls to avoid paths of unknown
/// providence ever being interpreted as relative. So lets do something similar
/// and pick an `EBADF`-based value outside of kernel reserved space.
enum { XCACHE_FILENO = -100 - EBADF };

/// identifiers for functions to request from the tracer
///
/// These are arbitrary numbers passed as the `request` argument to `ioctl`. For
/// convenience, they are chosen to be things that decode to an ASCII string
/// shorthand name.
enum call {
  CALL_HELLO = 0x6f6c6568,   ///< initial startup message from the spy
  CALL_OFF = 0x66666f,       ///< stop recording syscalls until seeing `CALL_ON`
  CALL_ON = 0x6e6f,          ///< start recording syscalls again
  CALL_SYSCONF = 0x666e6f63, ///< tracee called `sysconf`
  CALL_GETENV = 0x766e6567,  ///< tracee called `getenv`
  CALL_SETENV = 0x766e6573,  ///< tracee called `setenv`
  CALL_UNSETENV = 0x766e6575, ///< tracee called `unsetenv`
  CALL_PUTENV = 0x766e6570,   ///< tracee called `putenv`
  CALL_CLEARENV = 0x766e6563, ///< tracee called `clearenv`
  CALL_RNG_OFF =
      0x30676e72, ///< stop recording `getrandom` until seeing `CALL_RNG_ON`
  CALL_RNG_ON = 0x31676e72, ///< start recording `getrandom` again
};

/// the payload for a `CALL_SETENV`
typedef struct {
  uintptr_t name; ///< `name` parameter to `setenv`
  int overwrite;  ///< `overwrite` parameter to `setenv`
  int ret;        ///< return value from `setenv`
} setenv_t;

/// the payload for a `CALL_UNSETENV`
typedef struct {
  uintptr_t name; ///< `name` parameter to `unsetenv`
  int ret;        ///< return value from `unsetenv`
} unsetenv_t;

/// the payload for a `CALL_PUTENV`
typedef struct {
  uintptr_t string; ///< `string` parameter to `putenv`
  int ret;          ///< return value from `putenv`
} putenv_t;

/// get a string representation of a call number
///
/// @param callno Call number to convert
/// @return String representation
static inline const char *callno_to_str(enum call callno) {
  switch (callno) {
  case CALL_HELLO:
    return "\"hello\"";
  case CALL_OFF:
    return "\"off\"";
  case CALL_ON:
    return "\"on\"";
  case CALL_SYSCONF:
    return "\"sysconf\"";
  case CALL_GETENV:
    return "\"getenv\"";
  case CALL_SETENV:
    return "\"setenv\"";
  case CALL_UNSETENV:
    return "\"unsetenv\"";
  case CALL_PUTENV:
    return "\"putenv\"";
  case CALL_CLEARENV:
    return "\"clearenv\"";
  case CALL_RNG_OFF:
    return "\"RNG off\"";
  case CALL_RNG_ON:
    return "\"RNG on\"";
  }
  return "<unknown>";
}
