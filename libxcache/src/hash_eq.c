#include "hash_t.h"
#include <stdbool.h>

bool hash_eq(hash_t a, hash_t b) { return a.data == b.data; }
