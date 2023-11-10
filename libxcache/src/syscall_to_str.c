#include "syscall.h"
#include <sys/syscall.h>

const char *syscall_to_str(unsigned long number) {
#ifdef __NR_read
  if (number == __NR_read) {
    return "read";
  }
#endif
#ifdef __NR_write
  if (number == __NR_write) {
    return "write";
  }
#endif
#ifdef __NR_open
  if (number == __NR_open) {
    return "open";
  }
#endif
#ifdef __NR_close
  if (number == __NR_close) {
    return "close";
  }
#endif
#ifdef __NR_stat
  if (number == __NR_stat) {
    return "stat";
  }
#endif
#ifdef __NR_fstat
  if (number == __NR_fstat) {
    return "fstat";
  }
#endif
#ifdef __NR_lstat
  if (number == __NR_lstat) {
    return "lstat";
  }
#endif
#ifdef __NR_poll
  if (number == __NR_poll) {
    return "poll";
  }
#endif
#ifdef __NR_lseek
  if (number == __NR_lseek) {
    return "lseek";
  }
#endif
#ifdef __NR_mmap
  if (number == __NR_mmap) {
    return "mmap";
  }
#endif
#ifdef __NR_mprotect
  if (number == __NR_mprotect) {
    return "mprotect";
  }
#endif
#ifdef __NR_munmap
  if (number == __NR_munmap) {
    return "munmap";
  }
#endif
#ifdef __NR_brk
  if (number == __NR_brk) {
    return "brk";
  }
#endif
#ifdef __NR_rt_sigaction
  if (number == __NR_rt_sigaction) {
    return "rt_sigaction";
  }
#endif
#ifdef __NR_rt_sigprocmask
  if (number == __NR_rt_sigprocmask) {
    return "rt_sigprocmask";
  }
#endif
#ifdef __NR_rt_sigreturn
  if (number == __NR_rt_sigreturn) {
    return "rt_sigreturn";
  }
#endif
#ifdef __NR_ioctl
  if (number == __NR_ioctl) {
    return "ioctl";
  }
#endif
#ifdef __NR_pread64
  if (number == __NR_pread64) {
    return "pread64";
  }
#endif
#ifdef __NR_pwrite64
  if (number == __NR_pwrite64) {
    return "pwrite64";
  }
#endif
#ifdef __NR_readv
  if (number == __NR_readv) {
    return "readv";
  }
#endif
#ifdef __NR_writev
  if (number == __NR_writev) {
    return "writev";
  }
#endif
#ifdef __NR_access
  if (number == __NR_access) {
    return "access";
  }
#endif
#ifdef __NR_pipe
  if (number == __NR_pipe) {
    return "pipe";
  }
#endif
#ifdef __NR_select
  if (number == __NR_select) {
    return "select";
  }
#endif
#ifdef __NR_sched_yield
  if (number == __NR_sched_yield) {
    return "sched_yield";
  }
#endif
#ifdef __NR_mremap
  if (number == __NR_mremap) {
    return "mremap";
  }
#endif
#ifdef __NR_msync
  if (number == __NR_msync) {
    return "msync";
  }
#endif
#ifdef __NR_mincore
  if (number == __NR_mincore) {
    return "mincore";
  }
#endif
#ifdef __NR_madvise
  if (number == __NR_madvise) {
    return "madvise";
  }
#endif
#ifdef __NR_shmget
  if (number == __NR_shmget) {
    return "shmget";
  }
#endif
#ifdef __NR_shmat
  if (number == __NR_shmat) {
    return "shmat";
  }
#endif
#ifdef __NR_shmctl
  if (number == __NR_shmctl) {
    return "shmctl";
  }
#endif
#ifdef __NR_dup
  if (number == __NR_dup) {
    return "dup";
  }
#endif
#ifdef __NR_dup2
  if (number == __NR_dup2) {
    return "dup2";
  }
#endif
#ifdef __NR_pause
  if (number == __NR_pause) {
    return "pause";
  }
#endif
#ifdef __NR_nanosleep
  if (number == __NR_nanosleep) {
    return "nanosleep";
  }
#endif
#ifdef __NR_getitimer
  if (number == __NR_getitimer) {
    return "getitimer";
  }
#endif
#ifdef __NR_alarm
  if (number == __NR_alarm) {
    return "alarm";
  }
#endif
#ifdef __NR_setitimer
  if (number == __NR_setitimer) {
    return "setitimer";
  }
#endif
#ifdef __NR_getpid
  if (number == __NR_getpid) {
    return "getpid";
  }
#endif
#ifdef __NR_sendfile
  if (number == __NR_sendfile) {
    return "sendfile";
  }
#endif
#ifdef __NR_socket
  if (number == __NR_socket) {
    return "socket";
  }
#endif
#ifdef __NR_connect
  if (number == __NR_connect) {
    return "connect";
  }
#endif
#ifdef __NR_accept
  if (number == __NR_accept) {
    return "accept";
  }
#endif
#ifdef __NR_sendto
  if (number == __NR_sendto) {
    return "sendto";
  }
#endif
#ifdef __NR_recvfrom
  if (number == __NR_recvfrom) {
    return "recvfrom";
  }
#endif
#ifdef __NR_sendmsg
  if (number == __NR_sendmsg) {
    return "sendmsg";
  }
#endif
#ifdef __NR_recvmsg
  if (number == __NR_recvmsg) {
    return "recvmsg";
  }
#endif
#ifdef __NR_shutdown
  if (number == __NR_shutdown) {
    return "shutdown";
  }
#endif
#ifdef __NR_bind
  if (number == __NR_bind) {
    return "bind";
  }
#endif
#ifdef __NR_listen
  if (number == __NR_listen) {
    return "listen";
  }
#endif
#ifdef __NR_getsockname
  if (number == __NR_getsockname) {
    return "getsockname";
  }
#endif
#ifdef __NR_getpeername
  if (number == __NR_getpeername) {
    return "getpeername";
  }
#endif
#ifdef __NR_socketpair
  if (number == __NR_socketpair) {
    return "socketpair";
  }
#endif
#ifdef __NR_setsockopt
  if (number == __NR_setsockopt) {
    return "setsockopt";
  }
#endif
#ifdef __NR_getsockopt
  if (number == __NR_getsockopt) {
    return "getsockopt";
  }
#endif
#ifdef __NR_clone
  if (number == __NR_clone) {
    return "clone";
  }
#endif
#ifdef __NR_fork
  if (number == __NR_fork) {
    return "fork";
  }
#endif
#ifdef __NR_vfork
  if (number == __NR_vfork) {
    return "vfork";
  }
#endif
#ifdef __NR_execve
  if (number == __NR_execve) {
    return "execve";
  }
#endif
#ifdef __NR_exit
  if (number == __NR_exit) {
    return "exit";
  }
#endif
#ifdef __NR_wait4
  if (number == __NR_wait4) {
    return "wait4";
  }
#endif
#ifdef __NR_kill
  if (number == __NR_kill) {
    return "kill";
  }
#endif
#ifdef __NR_uname
  if (number == __NR_uname) {
    return "uname";
  }
#endif
#ifdef __NR_semget
  if (number == __NR_semget) {
    return "semget";
  }
#endif
#ifdef __NR_semop
  if (number == __NR_semop) {
    return "semop";
  }
#endif
#ifdef __NR_semctl
  if (number == __NR_semctl) {
    return "semctl";
  }
#endif
#ifdef __NR_shmdt
  if (number == __NR_shmdt) {
    return "shmdt";
  }
#endif
#ifdef __NR_msgget
  if (number == __NR_msgget) {
    return "msgget";
  }
#endif
#ifdef __NR_msgsnd
  if (number == __NR_msgsnd) {
    return "msgsnd";
  }
#endif
#ifdef __NR_msgrcv
  if (number == __NR_msgrcv) {
    return "msgrcv";
  }
#endif
#ifdef __NR_msgctl
  if (number == __NR_msgctl) {
    return "msgctl";
  }
#endif
#ifdef __NR_fcntl
  if (number == __NR_fcntl) {
    return "fcntl";
  }
#endif
#ifdef __NR_flock
  if (number == __NR_flock) {
    return "flock";
  }
#endif
#ifdef __NR_fsync
  if (number == __NR_fsync) {
    return "fsync";
  }
#endif
#ifdef __NR_fdatasync
  if (number == __NR_fdatasync) {
    return "fdatasync";
  }
#endif
#ifdef __NR_truncate
  if (number == __NR_truncate) {
    return "truncate";
  }
#endif
#ifdef __NR_ftruncate
  if (number == __NR_ftruncate) {
    return "ftruncate";
  }
#endif
#ifdef __NR_getdents
  if (number == __NR_getdents) {
    return "getdents";
  }
#endif
#ifdef __NR_getcwd
  if (number == __NR_getcwd) {
    return "getcwd";
  }
#endif
#ifdef __NR_chdir
  if (number == __NR_chdir) {
    return "chdir";
  }
#endif
#ifdef __NR_fchdir
  if (number == __NR_fchdir) {
    return "fchdir";
  }
#endif
#ifdef __NR_rename
  if (number == __NR_rename) {
    return "rename";
  }
#endif
#ifdef __NR_mkdir
  if (number == __NR_mkdir) {
    return "mkdir";
  }
#endif
#ifdef __NR_rmdir
  if (number == __NR_rmdir) {
    return "rmdir";
  }
#endif
#ifdef __NR_creat
  if (number == __NR_creat) {
    return "creat";
  }
#endif
#ifdef __NR_link
  if (number == __NR_link) {
    return "link";
  }
#endif
#ifdef __NR_unlink
  if (number == __NR_unlink) {
    return "unlink";
  }
#endif
#ifdef __NR_symlink
  if (number == __NR_symlink) {
    return "symlink";
  }
#endif
#ifdef __NR_readlink
  if (number == __NR_readlink) {
    return "readlink";
  }
#endif
#ifdef __NR_chmod
  if (number == __NR_chmod) {
    return "chmod";
  }
#endif
#ifdef __NR_fchmod
  if (number == __NR_fchmod) {
    return "fchmod";
  }
#endif
#ifdef __NR_chown
  if (number == __NR_chown) {
    return "chown";
  }
#endif
#ifdef __NR_fchown
  if (number == __NR_fchown) {
    return "fchown";
  }
#endif
#ifdef __NR_lchown
  if (number == __NR_lchown) {
    return "lchown";
  }
#endif
#ifdef __NR_umask
  if (number == __NR_umask) {
    return "umask";
  }
#endif
#ifdef __NR_gettimeofday
  if (number == __NR_gettimeofday) {
    return "gettimeofday";
  }
#endif
#ifdef __NR_getrlimit
  if (number == __NR_getrlimit) {
    return "getrlimit";
  }
#endif
#ifdef __NR_getrusage
  if (number == __NR_getrusage) {
    return "getrusage";
  }
#endif
#ifdef __NR_sysinfo
  if (number == __NR_sysinfo) {
    return "sysinfo";
  }
#endif
#ifdef __NR_times
  if (number == __NR_times) {
    return "times";
  }
#endif
#ifdef __NR_ptrace
  if (number == __NR_ptrace) {
    return "ptrace";
  }
#endif
#ifdef __NR_getuid
  if (number == __NR_getuid) {
    return "getuid";
  }
#endif
#ifdef __NR_syslog
  if (number == __NR_syslog) {
    return "syslog";
  }
#endif
#ifdef __NR_getgid
  if (number == __NR_getgid) {
    return "getgid";
  }
#endif
#ifdef __NR_setuid
  if (number == __NR_setuid) {
    return "setuid";
  }
#endif
#ifdef __NR_setgid
  if (number == __NR_setgid) {
    return "setgid";
  }
#endif
#ifdef __NR_geteuid
  if (number == __NR_geteuid) {
    return "geteuid";
  }
#endif
#ifdef __NR_getegid
  if (number == __NR_getegid) {
    return "getegid";
  }
#endif
#ifdef __NR_setpgid
  if (number == __NR_setpgid) {
    return "setpgid";
  }
#endif
#ifdef __NR_getppid
  if (number == __NR_getppid) {
    return "getppid";
  }
#endif
#ifdef __NR_getpgrp
  if (number == __NR_getpgrp) {
    return "getpgrp";
  }
#endif
#ifdef __NR_setsid
  if (number == __NR_setsid) {
    return "setsid";
  }
#endif
#ifdef __NR_setreuid
  if (number == __NR_setreuid) {
    return "setreuid";
  }
#endif
#ifdef __NR_setregid
  if (number == __NR_setregid) {
    return "setregid";
  }
#endif
#ifdef __NR_getgroups
  if (number == __NR_getgroups) {
    return "getgroups";
  }
#endif
#ifdef __NR_setgroups
  if (number == __NR_setgroups) {
    return "setgroups";
  }
#endif
#ifdef __NR_setresuid
  if (number == __NR_setresuid) {
    return "setresuid";
  }
#endif
#ifdef __NR_getresuid
  if (number == __NR_getresuid) {
    return "getresuid";
  }
#endif
#ifdef __NR_setresgid
  if (number == __NR_setresgid) {
    return "setresgid";
  }
#endif
#ifdef __NR_getresgid
  if (number == __NR_getresgid) {
    return "getresgid";
  }
#endif
#ifdef __NR_getpgid
  if (number == __NR_getpgid) {
    return "getpgid";
  }
#endif
#ifdef __NR_setfsuid
  if (number == __NR_setfsuid) {
    return "setfsuid";
  }
#endif
#ifdef __NR_setfsgid
  if (number == __NR_setfsgid) {
    return "setfsgid";
  }
#endif
#ifdef __NR_getsid
  if (number == __NR_getsid) {
    return "getsid";
  }
#endif
#ifdef __NR_capget
  if (number == __NR_capget) {
    return "capget";
  }
#endif
#ifdef __NR_capset
  if (number == __NR_capset) {
    return "capset";
  }
#endif
#ifdef __NR_rt_sigpending
  if (number == __NR_rt_sigpending) {
    return "rt_sigpending";
  }
#endif
#ifdef __NR_rt_sigtimedwait
  if (number == __NR_rt_sigtimedwait) {
    return "rt_sigtimedwait";
  }
#endif
#ifdef __NR_rt_sigqueueinfo
  if (number == __NR_rt_sigqueueinfo) {
    return "rt_sigqueueinfo";
  }
#endif
#ifdef __NR_rt_sigsuspend
  if (number == __NR_rt_sigsuspend) {
    return "rt_sigsuspend";
  }
#endif
#ifdef __NR_sigaltstack
  if (number == __NR_sigaltstack) {
    return "sigaltstack";
  }
#endif
#ifdef __NR_utime
  if (number == __NR_utime) {
    return "utime";
  }
#endif
#ifdef __NR_mknod
  if (number == __NR_mknod) {
    return "mknod";
  }
#endif
#ifdef __NR_uselib
  if (number == __NR_uselib) {
    return "uselib";
  }
#endif
#ifdef __NR_personality
  if (number == __NR_personality) {
    return "personality";
  }
#endif
#ifdef __NR_ustat
  if (number == __NR_ustat) {
    return "ustat";
  }
#endif
#ifdef __NR_statfs
  if (number == __NR_statfs) {
    return "statfs";
  }
#endif
#ifdef __NR_fstatfs
  if (number == __NR_fstatfs) {
    return "fstatfs";
  }
#endif
#ifdef __NR_sysfs
  if (number == __NR_sysfs) {
    return "sysfs";
  }
#endif
#ifdef __NR_getpriority
  if (number == __NR_getpriority) {
    return "getpriority";
  }
#endif
#ifdef __NR_setpriority
  if (number == __NR_setpriority) {
    return "setpriority";
  }
#endif
#ifdef __NR_sched_setparam
  if (number == __NR_sched_setparam) {
    return "sched_setparam";
  }
#endif
#ifdef __NR_sched_getparam
  if (number == __NR_sched_getparam) {
    return "sched_getparam";
  }
#endif
#ifdef __NR_sched_setscheduler
  if (number == __NR_sched_setscheduler) {
    return "sched_setscheduler";
  }
#endif
#ifdef __NR_sched_getscheduler
  if (number == __NR_sched_getscheduler) {
    return "sched_getscheduler";
  }
#endif
#ifdef __NR_sched_get_priority_max
  if (number == __NR_sched_get_priority_max) {
    return "sched_get_priority_max";
  }
#endif
#ifdef __NR_sched_get_priority_min
  if (number == __NR_sched_get_priority_min) {
    return "sched_get_priority_min";
  }
#endif
#ifdef __NR_sched_rr_get_interval
  if (number == __NR_sched_rr_get_interval) {
    return "sched_rr_get_interval";
  }
#endif
#ifdef __NR_mlock
  if (number == __NR_mlock) {
    return "mlock";
  }
#endif
#ifdef __NR_munlock
  if (number == __NR_munlock) {
    return "munlock";
  }
#endif
#ifdef __NR_mlockall
  if (number == __NR_mlockall) {
    return "mlockall";
  }
#endif
#ifdef __NR_munlockall
  if (number == __NR_munlockall) {
    return "munlockall";
  }
#endif
#ifdef __NR_vhangup
  if (number == __NR_vhangup) {
    return "vhangup";
  }
#endif
#ifdef __NR_modify_ldt
  if (number == __NR_modify_ldt) {
    return "modify_ldt";
  }
#endif
#ifdef __NR_pivot_root
  if (number == __NR_pivot_root) {
    return "pivot_root";
  }
#endif
#ifdef __NR__sysctl
  if (number == __NR__sysctl) {
    return "_sysctl";
  }
#endif
#ifdef __NR_prctl
  if (number == __NR_prctl) {
    return "prctl";
  }
#endif
#ifdef __NR_arch_prctl
  if (number == __NR_arch_prctl) {
    return "arch_prctl";
  }
#endif
#ifdef __NR_adjtimex
  if (number == __NR_adjtimex) {
    return "adjtimex";
  }
#endif
#ifdef __NR_setrlimit
  if (number == __NR_setrlimit) {
    return "setrlimit";
  }
#endif
#ifdef __NR_chroot
  if (number == __NR_chroot) {
    return "chroot";
  }
#endif
#ifdef __NR_sync
  if (number == __NR_sync) {
    return "sync";
  }
#endif
#ifdef __NR_acct
  if (number == __NR_acct) {
    return "acct";
  }
#endif
#ifdef __NR_settimeofday
  if (number == __NR_settimeofday) {
    return "settimeofday";
  }
#endif
#ifdef __NR_mount
  if (number == __NR_mount) {
    return "mount";
  }
#endif
#ifdef __NR_umount2
  if (number == __NR_umount2) {
    return "umount2";
  }
#endif
#ifdef __NR_swapon
  if (number == __NR_swapon) {
    return "swapon";
  }
#endif
#ifdef __NR_swapoff
  if (number == __NR_swapoff) {
    return "swapoff";
  }
#endif
#ifdef __NR_reboot
  if (number == __NR_reboot) {
    return "reboot";
  }
#endif
#ifdef __NR_sethostname
  if (number == __NR_sethostname) {
    return "sethostname";
  }
#endif
#ifdef __NR_setdomainname
  if (number == __NR_setdomainname) {
    return "setdomainname";
  }
#endif
#ifdef __NR_iopl
  if (number == __NR_iopl) {
    return "iopl";
  }
#endif
#ifdef __NR_ioperm
  if (number == __NR_ioperm) {
    return "ioperm";
  }
#endif
#ifdef __NR_create_module
  if (number == __NR_create_module) {
    return "create_module";
  }
#endif
#ifdef __NR_init_module
  if (number == __NR_init_module) {
    return "init_module";
  }
#endif
#ifdef __NR_delete_module
  if (number == __NR_delete_module) {
    return "delete_module";
  }
#endif
#ifdef __NR_get_kernel_syms
  if (number == __NR_get_kernel_syms) {
    return "get_kernel_syms";
  }
#endif
#ifdef __NR_query_module
  if (number == __NR_query_module) {
    return "query_module";
  }
#endif
#ifdef __NR_quotactl
  if (number == __NR_quotactl) {
    return "quotactl";
  }
#endif
#ifdef __NR_nfsservctl
  if (number == __NR_nfsservctl) {
    return "nfsservctl";
  }
#endif
#ifdef __NR_getpmsg
  if (number == __NR_getpmsg) {
    return "getpmsg";
  }
#endif
#ifdef __NR_putpmsg
  if (number == __NR_putpmsg) {
    return "putpmsg";
  }
#endif
#ifdef __NR_afs_syscall
  if (number == __NR_afs_syscall) {
    return "afs_syscall";
  }
#endif
#ifdef __NR_tuxcall
  if (number == __NR_tuxcall) {
    return "tuxcall";
  }
#endif
#ifdef __NR_security
  if (number == __NR_security) {
    return "security";
  }
#endif
#ifdef __NR_gettid
  if (number == __NR_gettid) {
    return "gettid";
  }
#endif
#ifdef __NR_readahead
  if (number == __NR_readahead) {
    return "readahead";
  }
#endif
#ifdef __NR_setxattr
  if (number == __NR_setxattr) {
    return "setxattr";
  }
#endif
#ifdef __NR_lsetxattr
  if (number == __NR_lsetxattr) {
    return "lsetxattr";
  }
#endif
#ifdef __NR_fsetxattr
  if (number == __NR_fsetxattr) {
    return "fsetxattr";
  }
#endif
#ifdef __NR_getxattr
  if (number == __NR_getxattr) {
    return "getxattr";
  }
#endif
#ifdef __NR_lgetxattr
  if (number == __NR_lgetxattr) {
    return "lgetxattr";
  }
#endif
#ifdef __NR_fgetxattr
  if (number == __NR_fgetxattr) {
    return "fgetxattr";
  }
#endif
#ifdef __NR_listxattr
  if (number == __NR_listxattr) {
    return "listxattr";
  }
#endif
#ifdef __NR_llistxattr
  if (number == __NR_llistxattr) {
    return "llistxattr";
  }
#endif
#ifdef __NR_flistxattr
  if (number == __NR_flistxattr) {
    return "flistxattr";
  }
#endif
#ifdef __NR_removexattr
  if (number == __NR_removexattr) {
    return "removexattr";
  }
#endif
#ifdef __NR_lremovexattr
  if (number == __NR_lremovexattr) {
    return "lremovexattr";
  }
#endif
#ifdef __NR_fremovexattr
  if (number == __NR_fremovexattr) {
    return "fremovexattr";
  }
#endif
#ifdef __NR_tkill
  if (number == __NR_tkill) {
    return "tkill";
  }
#endif
#ifdef __NR_time
  if (number == __NR_time) {
    return "time";
  }
#endif
#ifdef __NR_futex
  if (number == __NR_futex) {
    return "futex";
  }
#endif
#ifdef __NR_sched_setaffinity
  if (number == __NR_sched_setaffinity) {
    return "sched_setaffinity";
  }
#endif
#ifdef __NR_sched_getaffinity
  if (number == __NR_sched_getaffinity) {
    return "sched_getaffinity";
  }
#endif
#ifdef __NR_set_thread_area
  if (number == __NR_set_thread_area) {
    return "set_thread_area";
  }
#endif
#ifdef __NR_io_setup
  if (number == __NR_io_setup) {
    return "io_setup";
  }
#endif
#ifdef __NR_io_destroy
  if (number == __NR_io_destroy) {
    return "io_destroy";
  }
#endif
#ifdef __NR_io_getevents
  if (number == __NR_io_getevents) {
    return "io_getevents";
  }
#endif
#ifdef __NR_io_submit
  if (number == __NR_io_submit) {
    return "io_submit";
  }
#endif
#ifdef __NR_io_cancel
  if (number == __NR_io_cancel) {
    return "io_cancel";
  }
#endif
#ifdef __NR_get_thread_area
  if (number == __NR_get_thread_area) {
    return "get_thread_area";
  }
#endif
#ifdef __NR_lookup_dcookie
  if (number == __NR_lookup_dcookie) {
    return "lookup_dcookie";
  }
#endif
#ifdef __NR_epoll_create
  if (number == __NR_epoll_create) {
    return "epoll_create";
  }
#endif
#ifdef __NR_epoll_ctl_old
  if (number == __NR_epoll_ctl_old) {
    return "epoll_ctl_old";
  }
#endif
#ifdef __NR_epoll_wait_old
  if (number == __NR_epoll_wait_old) {
    return "epoll_wait_old";
  }
#endif
#ifdef __NR_remap_file_pages
  if (number == __NR_remap_file_pages) {
    return "remap_file_pages";
  }
#endif
#ifdef __NR_getdents64
  if (number == __NR_getdents64) {
    return "getdents64";
  }
#endif
#ifdef __NR_set_tid_address
  if (number == __NR_set_tid_address) {
    return "set_tid_address";
  }
#endif
#ifdef __NR_restart_syscall
  if (number == __NR_restart_syscall) {
    return "restart_syscall";
  }
#endif
#ifdef __NR_semtimedop
  if (number == __NR_semtimedop) {
    return "semtimedop";
  }
#endif
#ifdef __NR_fadvise64
  if (number == __NR_fadvise64) {
    return "fadvise64";
  }
#endif
#ifdef __NR_timer_create
  if (number == __NR_timer_create) {
    return "timer_create";
  }
#endif
#ifdef __NR_timer_settime
  if (number == __NR_timer_settime) {
    return "timer_settime";
  }
#endif
#ifdef __NR_timer_gettime
  if (number == __NR_timer_gettime) {
    return "timer_gettime";
  }
#endif
#ifdef __NR_timer_getoverrun
  if (number == __NR_timer_getoverrun) {
    return "timer_getoverrun";
  }
#endif
#ifdef __NR_timer_delete
  if (number == __NR_timer_delete) {
    return "timer_delete";
  }
#endif
#ifdef __NR_clock_settime
  if (number == __NR_clock_settime) {
    return "clock_settime";
  }
#endif
#ifdef __NR_clock_gettime
  if (number == __NR_clock_gettime) {
    return "clock_gettime";
  }
#endif
#ifdef __NR_clock_getres
  if (number == __NR_clock_getres) {
    return "clock_getres";
  }
#endif
#ifdef __NR_clock_nanosleep
  if (number == __NR_clock_nanosleep) {
    return "clock_nanosleep";
  }
#endif
#ifdef __NR_exit_group
  if (number == __NR_exit_group) {
    return "exit_group";
  }
#endif
#ifdef __NR_epoll_wait
  if (number == __NR_epoll_wait) {
    return "epoll_wait";
  }
#endif
#ifdef __NR_epoll_ctl
  if (number == __NR_epoll_ctl) {
    return "epoll_ctl";
  }
#endif
#ifdef __NR_tgkill
  if (number == __NR_tgkill) {
    return "tgkill";
  }
#endif
#ifdef __NR_utimes
  if (number == __NR_utimes) {
    return "utimes";
  }
#endif
#ifdef __NR_vserver
  if (number == __NR_vserver) {
    return "vserver";
  }
#endif
#ifdef __NR_mbind
  if (number == __NR_mbind) {
    return "mbind";
  }
#endif
#ifdef __NR_set_mempolicy
  if (number == __NR_set_mempolicy) {
    return "set_mempolicy";
  }
#endif
#ifdef __NR_get_mempolicy
  if (number == __NR_get_mempolicy) {
    return "get_mempolicy";
  }
#endif
#ifdef __NR_mq_open
  if (number == __NR_mq_open) {
    return "mq_open";
  }
#endif
#ifdef __NR_mq_unlink
  if (number == __NR_mq_unlink) {
    return "mq_unlink";
  }
#endif
#ifdef __NR_mq_timedsend
  if (number == __NR_mq_timedsend) {
    return "mq_timedsend";
  }
#endif
#ifdef __NR_mq_timedreceive
  if (number == __NR_mq_timedreceive) {
    return "mq_timedreceive";
  }
#endif
#ifdef __NR_mq_notify
  if (number == __NR_mq_notify) {
    return "mq_notify";
  }
#endif
#ifdef __NR_mq_getsetattr
  if (number == __NR_mq_getsetattr) {
    return "mq_getsetattr";
  }
#endif
#ifdef __NR_kexec_load
  if (number == __NR_kexec_load) {
    return "kexec_load";
  }
#endif
#ifdef __NR_waitid
  if (number == __NR_waitid) {
    return "waitid";
  }
#endif
#ifdef __NR_add_key
  if (number == __NR_add_key) {
    return "add_key";
  }
#endif
#ifdef __NR_request_key
  if (number == __NR_request_key) {
    return "request_key";
  }
#endif
#ifdef __NR_keyctl
  if (number == __NR_keyctl) {
    return "keyctl";
  }
#endif
#ifdef __NR_ioprio_set
  if (number == __NR_ioprio_set) {
    return "ioprio_set";
  }
#endif
#ifdef __NR_ioprio_get
  if (number == __NR_ioprio_get) {
    return "ioprio_get";
  }
#endif
#ifdef __NR_inotify_init
  if (number == __NR_inotify_init) {
    return "inotify_init";
  }
#endif
#ifdef __NR_inotify_add_watch
  if (number == __NR_inotify_add_watch) {
    return "inotify_add_watch";
  }
#endif
#ifdef __NR_inotify_rm_watch
  if (number == __NR_inotify_rm_watch) {
    return "inotify_rm_watch";
  }
#endif
#ifdef __NR_migrate_pages
  if (number == __NR_migrate_pages) {
    return "migrate_pages";
  }
#endif
#ifdef __NR_openat
  if (number == __NR_openat) {
    return "openat";
  }
#endif
#ifdef __NR_mkdirat
  if (number == __NR_mkdirat) {
    return "mkdirat";
  }
#endif
#ifdef __NR_mknodat
  if (number == __NR_mknodat) {
    return "mknodat";
  }
#endif
#ifdef __NR_fchownat
  if (number == __NR_fchownat) {
    return "fchownat";
  }
#endif
#ifdef __NR_futimesat
  if (number == __NR_futimesat) {
    return "futimesat";
  }
#endif
#ifdef __NR_newfstatat
  if (number == __NR_newfstatat) {
    return "newfstatat";
  }
#endif
#ifdef __NR_unlinkat
  if (number == __NR_unlinkat) {
    return "unlinkat";
  }
#endif
#ifdef __NR_renameat
  if (number == __NR_renameat) {
    return "renameat";
  }
#endif
#ifdef __NR_linkat
  if (number == __NR_linkat) {
    return "linkat";
  }
#endif
#ifdef __NR_symlinkat
  if (number == __NR_symlinkat) {
    return "symlinkat";
  }
#endif
#ifdef __NR_readlinkat
  if (number == __NR_readlinkat) {
    return "readlinkat";
  }
#endif
#ifdef __NR_fchmodat
  if (number == __NR_fchmodat) {
    return "fchmodat";
  }
#endif
#ifdef __NR_faccessat
  if (number == __NR_faccessat) {
    return "faccessat";
  }
#endif
#ifdef __NR_pselect6
  if (number == __NR_pselect6) {
    return "pselect6";
  }
#endif
#ifdef __NR_ppoll
  if (number == __NR_ppoll) {
    return "ppoll";
  }
#endif
#ifdef __NR_unshare
  if (number == __NR_unshare) {
    return "unshare";
  }
#endif
#ifdef __NR_set_robust_list
  if (number == __NR_set_robust_list) {
    return "set_robust_list";
  }
#endif
#ifdef __NR_get_robust_list
  if (number == __NR_get_robust_list) {
    return "get_robust_list";
  }
#endif
#ifdef __NR_splice
  if (number == __NR_splice) {
    return "splice";
  }
#endif
#ifdef __NR_tee
  if (number == __NR_tee) {
    return "tee";
  }
#endif
#ifdef __NR_sync_file_range
  if (number == __NR_sync_file_range) {
    return "sync_file_range";
  }
#endif
#ifdef __NR_vmsplice
  if (number == __NR_vmsplice) {
    return "vmsplice";
  }
#endif
#ifdef __NR_move_pages
  if (number == __NR_move_pages) {
    return "move_pages";
  }
#endif
#ifdef __NR_utimensat
  if (number == __NR_utimensat) {
    return "utimensat";
  }
#endif
#ifdef __NR_epoll_pwait
  if (number == __NR_epoll_pwait) {
    return "epoll_pwait";
  }
#endif
#ifdef __NR_signalfd
  if (number == __NR_signalfd) {
    return "signalfd";
  }
#endif
#ifdef __NR_timerfd_create
  if (number == __NR_timerfd_create) {
    return "timerfd_create";
  }
#endif
#ifdef __NR_eventfd
  if (number == __NR_eventfd) {
    return "eventfd";
  }
#endif
#ifdef __NR_fallocate
  if (number == __NR_fallocate) {
    return "fallocate";
  }
#endif
#ifdef __NR_timerfd_settime
  if (number == __NR_timerfd_settime) {
    return "timerfd_settime";
  }
#endif
#ifdef __NR_timerfd_gettime
  if (number == __NR_timerfd_gettime) {
    return "timerfd_gettime";
  }
#endif
#ifdef __NR_accept4
  if (number == __NR_accept4) {
    return "accept4";
  }
#endif
#ifdef __NR_signalfd4
  if (number == __NR_signalfd4) {
    return "signalfd4";
  }
#endif
#ifdef __NR_eventfd2
  if (number == __NR_eventfd2) {
    return "eventfd2";
  }
#endif
#ifdef __NR_epoll_create1
  if (number == __NR_epoll_create1) {
    return "epoll_create1";
  }
#endif
#ifdef __NR_dup3
  if (number == __NR_dup3) {
    return "dup3";
  }
#endif
#ifdef __NR_pipe2
  if (number == __NR_pipe2) {
    return "pipe2";
  }
#endif
#ifdef __NR_inotify_init1
  if (number == __NR_inotify_init1) {
    return "inotify_init1";
  }
#endif
#ifdef __NR_preadv
  if (number == __NR_preadv) {
    return "preadv";
  }
#endif
#ifdef __NR_pwritev
  if (number == __NR_pwritev) {
    return "pwritev";
  }
#endif
#ifdef __NR_rt_tgsigqueueinfo
  if (number == __NR_rt_tgsigqueueinfo) {
    return "rt_tgsigqueueinfo";
  }
#endif
#ifdef __NR_perf_event_open
  if (number == __NR_perf_event_open) {
    return "perf_event_open";
  }
#endif
#ifdef __NR_recvmmsg
  if (number == __NR_recvmmsg) {
    return "recvmmsg";
  }
#endif
#ifdef __NR_fanotify_init
  if (number == __NR_fanotify_init) {
    return "fanotify_init";
  }
#endif
#ifdef __NR_fanotify_mark
  if (number == __NR_fanotify_mark) {
    return "fanotify_mark";
  }
#endif
#ifdef __NR_prlimit64
  if (number == __NR_prlimit64) {
    return "prlimit64";
  }
#endif
#ifdef __NR_name_to_handle_at
  if (number == __NR_name_to_handle_at) {
    return "name_to_handle_at";
  }
#endif
#ifdef __NR_open_by_handle_at
  if (number == __NR_open_by_handle_at) {
    return "open_by_handle_at";
  }
#endif
#ifdef __NR_clock_adjtime
  if (number == __NR_clock_adjtime) {
    return "clock_adjtime";
  }
#endif
#ifdef __NR_syncfs
  if (number == __NR_syncfs) {
    return "syncfs";
  }
#endif
#ifdef __NR_sendmmsg
  if (number == __NR_sendmmsg) {
    return "sendmmsg";
  }
#endif
#ifdef __NR_setns
  if (number == __NR_setns) {
    return "setns";
  }
#endif
#ifdef __NR_getcpu
  if (number == __NR_getcpu) {
    return "getcpu";
  }
#endif
#ifdef __NR_process_vm_readv
  if (number == __NR_process_vm_readv) {
    return "process_vm_readv";
  }
#endif
#ifdef __NR_process_vm_writev
  if (number == __NR_process_vm_writev) {
    return "process_vm_writev";
  }
#endif
#ifdef __NR_kcmp
  if (number == __NR_kcmp) {
    return "kcmp";
  }
#endif
#ifdef __NR_finit_module
  if (number == __NR_finit_module) {
    return "finit_module";
  }
#endif
#ifdef __NR_sched_setattr
  if (number == __NR_sched_setattr) {
    return "sched_setattr";
  }
#endif
#ifdef __NR_sched_getattr
  if (number == __NR_sched_getattr) {
    return "sched_getattr";
  }
#endif
#ifdef __NR_renameat2
  if (number == __NR_renameat2) {
    return "renameat2";
  }
#endif
#ifdef __NR_seccomp
  if (number == __NR_seccomp) {
    return "seccomp";
  }
#endif
#ifdef __NR_getrandom
  if (number == __NR_getrandom) {
    return "getrandom";
  }
#endif
#ifdef __NR_memfd_create
  if (number == __NR_memfd_create) {
    return "memfd_create";
  }
#endif
#ifdef __NR_kexec_file_load
  if (number == __NR_kexec_file_load) {
    return "kexec_file_load";
  }
#endif
#ifdef __NR_bpf
  if (number == __NR_bpf) {
    return "bpf";
  }
#endif
#ifdef __NR_execveat
  if (number == __NR_execveat) {
    return "execveat";
  }
#endif
#ifdef __NR_userfaultfd
  if (number == __NR_userfaultfd) {
    return "userfaultfd";
  }
#endif
#ifdef __NR_membarrier
  if (number == __NR_membarrier) {
    return "membarrier";
  }
#endif
#ifdef __NR_mlock2
  if (number == __NR_mlock2) {
    return "mlock2";
  }
#endif
#ifdef __NR_copy_file_range
  if (number == __NR_copy_file_range) {
    return "copy_file_range";
  }
#endif
#ifdef __NR_preadv2
  if (number == __NR_preadv2) {
    return "preadv2";
  }
#endif
#ifdef __NR_pwritev2
  if (number == __NR_pwritev2) {
    return "pwritev2";
  }
#endif
#ifdef __NR_pkey_mprotect
  if (number == __NR_pkey_mprotect) {
    return "pkey_mprotect";
  }
#endif
#ifdef __NR_pkey_alloc
  if (number == __NR_pkey_alloc) {
    return "pkey_alloc";
  }
#endif
#ifdef __NR_pkey_free
  if (number == __NR_pkey_free) {
    return "pkey_free";
  }
#endif
#ifdef __NR_statx
  if (number == __NR_statx) {
    return "statx";
  }
#endif
#ifdef __NR_io_pgetevents
  if (number == __NR_io_pgetevents) {
    return "io_pgetevents";
  }
#endif
#ifdef __NR_rseq
  if (number == __NR_rseq) {
    return "rseq";
  }
#endif
#ifdef __NR_pidfd_send_signal
  if (number == __NR_pidfd_send_signal) {
    return "pidfd_send_signal";
  }
#endif
#ifdef __NR_io_uring_setup
  if (number == __NR_io_uring_setup) {
    return "io_uring_setup";
  }
#endif
#ifdef __NR_io_uring_enter
  if (number == __NR_io_uring_enter) {
    return "io_uring_enter";
  }
#endif
#ifdef __NR_io_uring_register
  if (number == __NR_io_uring_register) {
    return "io_uring_register";
  }
#endif
#ifdef __NR_open_tree
  if (number == __NR_open_tree) {
    return "open_tree";
  }
#endif
#ifdef __NR_move_mount
  if (number == __NR_move_mount) {
    return "move_mount";
  }
#endif
#ifdef __NR_fsopen
  if (number == __NR_fsopen) {
    return "fsopen";
  }
#endif
#ifdef __NR_fsconfig
  if (number == __NR_fsconfig) {
    return "fsconfig";
  }
#endif
#ifdef __NR_fsmount
  if (number == __NR_fsmount) {
    return "fsmount";
  }
#endif
#ifdef __NR_fspick
  if (number == __NR_fspick) {
    return "fspick";
  }
#endif
#ifdef __NR_pidfd_open
  if (number == __NR_pidfd_open) {
    return "pidfd_open";
  }
#endif
#ifdef __NR_clone3
  if (number == __NR_clone3) {
    return "clone3";
  }
#endif
#ifdef __NR_close_range
  if (number == __NR_close_range) {
    return "close_range";
  }
#endif
#ifdef __NR_openat2
  if (number == __NR_openat2) {
    return "openat2";
  }
#endif
#ifdef __NR_pidfd_getfd
  if (number == __NR_pidfd_getfd) {
    return "pidfd_getfd";
  }
#endif
#ifdef __NR_faccessat2
  if (number == __NR_faccessat2) {
    return "faccessat2";
  }
#endif
#ifdef __NR_process_madvise
  if (number == __NR_process_madvise) {
    return "process_madvise";
  }
#endif
#ifdef __NR_epoll_pwait2
  if (number == __NR_epoll_pwait2) {
    return "epoll_pwait2";
  }
#endif
#ifdef __NR_mount_setattr
  if (number == __NR_mount_setattr) {
    return "mount_setattr";
  }
#endif
#ifdef __NR_quotactl_fd
  if (number == __NR_quotactl_fd) {
    return "quotactl_fd";
  }
#endif
#ifdef __NR_landlock_create_ruleset
  if (number == __NR_landlock_create_ruleset) {
    return "landlock_create_ruleset";
  }
#endif
#ifdef __NR_landlock_add_rule
  if (number == __NR_landlock_add_rule) {
    return "landlock_add_rule";
  }
#endif
#ifdef __NR_landlock_restrict_self
  if (number == __NR_landlock_restrict_self) {
    return "landlock_restrict_self";
  }
#endif
#ifdef __NR_memfd_secret
  if (number == __NR_memfd_secret) {
    return "memfd_secret";
  }
#endif
#ifdef __NR_process_mrelease
  if (number == __NR_process_mrelease) {
    return "process_mrelease";
  }
#endif
#ifdef __NR_futex_waitv
  if (number == __NR_futex_waitv) {
    return "futex_waitv";
  }
#endif
#ifdef __NR_set_mempolicy_home_node
  if (number == __NR_set_mempolicy_home_node) {
    return "set_mempolicy_home_node";
  }
#endif
#ifdef __NR_cachestat
  if (number == __NR_cachestat) {
    return "cachestat";
  }
#endif
#ifdef __NR_fchmodat2
  if (number == __NR_fchmodat2) {
    return "fchmodat2";
  }
#endif
#ifdef __NR_map_shadow_stack
  if (number == __NR_map_shadow_stack) {
    return "map_shadow_stack";
  }
#endif
#ifdef __NR_futex_wake
  if (number == __NR_futex_wake) {
    return "futex_wake";
  }
#endif
#ifdef __NR_futex_wait
  if (number == __NR_futex_wait) {
    return "futex_wait";
  }
#endif
#ifdef __NR_futex_requeue
  if (number == __NR_futex_requeue) {
    return "futex_requeue";
  }
#endif

  return "<unknown>";
}
