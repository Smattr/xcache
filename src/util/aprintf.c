/* asprintf-alike. As far as valgrind can tell, asprintf leaks memory, which
 * makes it a little dangerous to call in the unconstrained context of xcache.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *aprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    /* We need to copy the va_list because a call to vsnprintf invalidates it
     * and we need to invoke it twice.
     */
    va_list ap_copy;
    va_copy(ap_copy, ap);

    /* Figure out how many bytes we need for this string. */
    size_t len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    char *s = malloc(sizeof(char) * (len + 1));
    if (s == NULL)
        return NULL;

    /* OK, let's do this thing. */
    size_t len2 __attribute__((unused)) = vsprintf(s, fmt, ap_copy);
    assert(len2 == len);
    va_end(ap_copy);

    return s;
}
