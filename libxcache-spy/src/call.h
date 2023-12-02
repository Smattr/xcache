#pragma once

#include "../../common/compiler.h"

/** send a message to the tracer
 *
 * This function returns no indication of success or failure because there is no
 * easy way of determining whether the tracer successfully received the call.
 *
 * \param callno A `CALL_*` value from ../../common/proccall.h
 * \param arg Any parameter the call takes or else `NULL`
 */
INTERNAL void call(unsigned long callno, const char *arg);
