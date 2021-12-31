xcache – ccache for everything
==============================
Xcache is a tool for tracing and then caching the outputs of arbitrary programs.
You run something once with xcache observing it. If its inputs have not changed
the next time you execute it, xcache can retrieve the outputs it cached from the
previous run and avoid re-executing the program.

The purpose of this is to save time spent executing programs that do complicated
calculations. The prime example is a compiler. Running a compiler on a large
program can take a long time due to time spent computing optimisations. Xcache
can save you spending this time repeatedly in future compilations.

If you are familiar with ccache_, xcache tries to do the same thing but for any
program, not just your C compiler.

How Does It Work?
-----------------
Xcache uses a combination of seccomp_ and ptrace_. It runs the target process
(the “tracee”) as if xcache were a debugger observing its operations. Every time
the tracee makes a relevant system call, xcache intercepts it and records its
effect.

When asked to run a target process it has seen before, xcache locates its
transcript from the previous run and determines whether it can be replayed or
whether conditions have changed, requiring re-execution. If replay is possible,
the replay is generally much faster than re-executing the target process.

To learn more, read the source or ask me questions.

.. _ccache: https://ccache.samba.org/
.. _ptrace: https://en.wikipedia.org/wiki/Ptrace
.. _seccomp: https://en.wikipedia.org/wiki/Seccomp
