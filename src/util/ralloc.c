#include <stdlib.h>
#include "../util.h"

void *ralloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (p == NULL)
        free(ptr);
    return p;
}
