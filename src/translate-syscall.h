#ifndef _XCACHE_TRANSLATE_SYSCALL_H_
#define _XCACHE_TRANSLATE_SYSCALL_H_

/* Translate a syscall number into its name as a string. Used for debugging
 * messages. Do not free the returned pointer.
 */
const char *translate_syscall(int sysno);

#endif
