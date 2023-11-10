#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <errno.h>

int sysexit(proc_t *proc) {

  int rc = 0;

  const unsigned long syscall_no = peek_syscall_no(proc->pid);
  DEBUG("sysexit %s (%lu)", syscall_to_str(syscall_no), syscall_no);

  rc = ENOTSUP;
  return rc;
}
