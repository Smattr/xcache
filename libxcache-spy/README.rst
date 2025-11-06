libxcache-spy
=============
Tracee-side agent for monitoring userspace actions.

While Xcache – or, more specifically, libxcache – is tracing a subprocess and
monitoring its syscalls, it cannot easily intercept operations that happen
without kernel interaction. An example of this is ``getenv``, a libc call that
is relevant to tracing but happens purely in userspace.

To deal with this, libxcache injects this library into the subprocess via
``LD_PRELOAD``. It can intercept libc calls via its own wrappers and then relay
details back to its parent, libxcache. In doing this, it must try to only use
syscalls that are ignored by tracing (e.g. ``read`` and ``write``) because its
actions are indistinguishable from those of the tracee.
