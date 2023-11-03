#include "debug.h"
#include <linux/version.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <xcache/record.h>

unsigned xc_record_modes(unsigned request) {

  // mask down to known modes
  unsigned answer = request & XC_MODE_AUTO;

// if we were compiled on an older kernel
#if LINUX_VERSION_MAJOR < 3 ||                                                 \
    (LINUX_VERSION_MAJOR == 3 && LINUX_VERSION_MINOR < 5)
  answer &= ~(XC_EARLY_SECCOMP | XC_LATE_SECCOMP);
#endif

  // get kernel version
  struct utsname name;
  if (ERROR(uname(&name) < 0))
    return 0;
  int major = 0;
  int minor = 0;
  if (sscanf(name.release, "%d.%d.", &major, &minor) != 2)
    return 0;

  // pre-syscall seccomp behaviour exists in Linux [3.5, 4.8)
  if (major < 3 || major > 4 || (major == 3 && minor < 5) ||
      (major == 4 && minor > 7))
    answer &= ~XC_EARLY_SECCOMP;

  // mid-syscall seccomp behaviour exists in Linux ≥ 4.8
  if (major < 4 || (major == 4 && minor < 8))
    answer &= ~XC_LATE_SECCOMP;

  return answer;
}
