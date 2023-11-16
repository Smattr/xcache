#include "hash_t.h"
#include <stdbool.h>

bool hash_eq(const hash_t a, const hash_t b) { return a.data == b.data; }
