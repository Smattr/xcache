#include "debug.h"
#include <linux/version.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <xcache/record.h>

unsigned xc_record_modes(unsigned request) {

  // mask down to known modes
  unsigned answer = request & XC_MODE_AUTO;

// if we were compiled on an older kernel
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
  answer &= ~(XC_EARLY_SECCOMP | XC_LATE_SECCOMP);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
  answer &= ~XC_LATE_SECCOMP;
#else
  answer &= ~XC_EARLY_SECCOMP;
#endif

  // get kernel version
  struct utsname name;
  if (ERROR(uname(&name) < 0))
    return 0;
  int major = 0;
  int patch = 0;
  int sub = 0;
  if (sscanf(name.release, "%d.%d.%d", &major, &patch, &sub) != 3)
    return 0;
  const int v = KERNEL_VERSION(major, patch, sub);

  // pre-syscall seccomp behaviour exists in Linux [3.5, 4.8)
  if (v < KERNEL_VERSION(3, 5, 0) || v >= KERNEL_VERSION(4, 8, 0))
    answer &= ~XC_EARLY_SECCOMP;

  // mid-syscall seccomp behaviour exists in Linux ≥ 4.8
  if (v < KERNEL_VERSION(4, 8, 0))
    answer &= ~XC_LATE_SECCOMP;

  return answer;
}
