#include <assert.h>
#include <stdlib.h>
#include "../util.h"

unsigned int hash(size_t limit, const char *s) {
    unsigned int h = 0;
    assert(s != NULL);
    for (; *s != '\0'; s++) {
        h = *s + 63 * h;
    }
    return h % limit;
}
