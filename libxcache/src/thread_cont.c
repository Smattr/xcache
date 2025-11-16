#include "thread_t.h"

int thread_cont(thread_t thread) { return thread_signal(thread, 0); }
