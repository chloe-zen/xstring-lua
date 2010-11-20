/* xstring public C API
 * by Chip Salzenberg <chip@pobox.com>
 */

#ifndef XSTRING_H
#define XSTRING_H

#include <lua.h>

#ifdef _cplusplus
extern "C" {
#endif

typedef struct xsbuf {
    unsigned refs;
    const char *data;
    size_t size;
} xsbuf_t;

xsbuf_t *    xsbuf_new(const char *data, size_t size);
xsbuf_t *    xsbuf_use(xsbuf_t *b);                          /* xsbuf's refcount ++ */
void 	     xsbuf_discard(xsbuf_t *b);                      /* xsbuf's refcount -- */
void 	     xsbuf_kill(xsbuf_t *b);                         /* "those data are gone" */
int          xsbuf_valid(xsbuf_t *b);			     /* "are the data still there?" */

int          xstring_new(lua_State *L, xsbuf_t *b);          /* 0=fail 1=ok */
xsbuf_t *    xstring_toxsbuf(lua_State *L, int n);           /* does not throw */
const char * xstring_toaddr(lua_State *L, int n, size_t *l); /* does not throw */
const char * xstring_check(lua_State *L, int n, size_t *l);  /* throws */

#ifdef _cplusplus
} /* extern "C" */
#endif

#endif /* XSTRING_H */
