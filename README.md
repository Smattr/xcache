*_This is currently a work-in-progress and nothing should be expected to compile, much less run._*

# xcache

Xcache is a tool for tracing and then caching the outputs of arbitrary
programs. You run something once with xcache observing it. If its inputs have
not changed the next time you execute it, xcache can retrieve the outputs it
cached from the previous run and avoid re-executing the program.

The purpose of this is to save time spent executing programs that do
complicated calculations. The prime example is a compiler. Running a compiler
on a large program can take a long time due to time spent computing
optimisations. Xcache can save you spending this time repeatedly in future
compilations.

If you are familiar with [ccache](https://ccache.samba.org/), xcache tries to
do the same thing but for any program, not just your C compiler.

## How Does It Work?

Internally xcache uses the Linux [ptrace](http://linux.die.net/man/2/ptrace)
infrastructure. It traps the target (the program being traced) every time it
makes a relevant syscall and records any new input or output. After a successful
run, a cache entry is stored on disk with metadata in a
[SQLite](https://www.sqlite.org/) database.

When invoked with any target program, xcache first checks its cache for a
successful previous execution of this program in the same environment. If the
inputs have changed since this entry was made, the entry is discarded and the
program is re-run. If, however, the inputs have not changed, the cached entry
can be retrieved saving you runtime.

To learn more, read the source.

## Status

Almost nothing works at the moment. I'm still hacking fairly heavily on the
code and comments are sparse. This README will be updated when the code is in
better shape.

## Feedback and Support

I'm happy to take questions via email or respond to issues through the
Bitbucket tracker. However, please bear in mind that this is not my full time
job and I may take a little while to respond.

## Testing

Some limited unit tests are implemented inline. These are built as the
executable, xcache-tests. Some even more limited integration tests are in the
tests/ subdirectory.

## xxxcache

You'll find a quick-and-dirty Python implementation in xxxcache/. This has
primarily been used during development and debugging, but can be used as a
complete caching solution in limited circumstances. Specifically, if the target
you're tracing is well understood and has constrained inputs this could work
well for you. Note that it can be difficult to get solid gains with xxxcache if
you're tracing a native process simply because the startup of the Python
runtime can dwarf any other factors.

You can find more about xxxcache in xxxcache/README.md.

## Legal

Copyright (c) 2015, Matthew Fernandez
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Matthew Fernandez nor the
  names of other contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MATTHEW FERNANDEZ BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
