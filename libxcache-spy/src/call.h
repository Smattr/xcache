#pragma once

#include "../../common/compiler.h"

/// send a message to the tracer
///
/// This function returns no indication of success or failure because there is
/// no easy way of determining whether the tracer successfully received the
/// call.
///
/// @param callno A `CALL_*` value from ../../common/proccall.h
/// @param arg Any parameter the call takes or else `NULL`
INTERNAL void call(unsigned long callno, const void *arg);

/// disabled recording of syscalls
///
/// This is a shortcut for `call(CALL_OFF, NULL)` but handles the possibility of
/// recursing into the caller or another function that calls `call_off` while
/// syscall recording is disabled.
INTERNAL void call_off(void);

/// disabled recording of syscalls
///
/// This is a shortcut for `call(CALL_ON, NULL)` but handles the possibility of
/// recursing into the caller or another function that calls `call_on` while
/// syscall recording is disabled.
INTERNAL void call_on(void);
