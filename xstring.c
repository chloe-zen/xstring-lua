/* xstring core code
 * by Chip Salzenberg <chip@pobox.com>
 */

#include "xstring.h"
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define _buf_valid(B)    ((B) && (B)->data)
#define _xstr_valid(XS)  _buf_valid((XS)->buf) /* currently not possible for it to be null */

#define checkxs(L,I)     (xstr_t *)luaL_checkudata(L, I, xstr_mt)

static const char xstr_mt[]    = "xstring";

/*================================================================
 * xstr
 */

typedef struct xstr {
    xsbuf_t *buf;
    size_t pos, len;
} xstr_t;


/*----------------------------------------------------------------
 * PUBLIC C API AND GC
 */

/*
 * Create, invalidate, use (++refcount), discard (--refcount), an xstrbuf.
 * In a concession to usability, it is created with a refcount of 1.
 */

#define XSBUF_USE(B)      (++(B)->refs, (B))
#define XSBUF_DISCARD(B)  do { if (--(B)->refs == 0) free(B); } while (0)

xsbuf_t *xsbuf_new(const char *data, size_t size) {
    xsbuf_t *b = malloc(sizeof *b);
    if (b) {
	/* normalize empty string to be non-null with length of zero */
	if (!data || !size) {
	    data = "";
	    size = 0;
	}
	b->refs = 1;
	b->data = data;
	b->size = size;
    }
    return b;
}

xsbuf_t *xsbuf_use(xsbuf_t *b) {
    if (b)
	XSBUF_USE(b);
    return b;
}

void xsbuf_discard(xsbuf_t *b) {
    if (b)
	XSBUF_DISCARD(b);
}

void xsbuf_kill(xsbuf_t *b) {
    if (b) {
	b->data = NULL;
	b->size = 0;
    }
}

int xsbuf_valid(xsbuf_t *b) {
    return _buf_valid(b);
}


/*
 * Create a new xstr referring to the given strbuf, push the udata on the stack,
 *   and return 0.  If something goes wrong, return -1.  XXX: error enum?
 *
 * Note that the C code should hold onto the xstr_t pointer,
 *   and is responsible to call xstr_discard() when the target
 *   data are no longer available; this step is not optional.
 */

static xstr_t *_xstr_newptr(lua_State *L, xsbuf_t *b) {
    xstr_t *xs;
    if (!b)
	return NULL;
    xs = lua_newuserdata(L, sizeof *xs);
    xs->buf = XSBUF_USE(b);
    xs->pos = 0;
    xs->len = b->size;
    luaL_getmetatable(L, xstr_mt);
    lua_setmetatable(L, -2);
    return xs;
}

int xstring_new(lua_State *L, xsbuf_t *b) {
    return _xstr_newptr(L, b) != NULL;
}

/* xstring.__gc */
static int xstr_gc(lua_State *L) {
    xstr_t *xs = checkxs(L, 1);
    if (xs->buf) {
	XSBUF_DISCARD(xs->buf);
	xs->buf = NULL;
    }
    return 0;
}

/* internal function to get the xstr_t pointer, or NULL */
static xstr_t *_xstr_get(lua_State *L, int n) {
    void *ud;
    if (!(ud = lua_touserdata(L, n)))
	return NULL;
    if (!lua_getmetatable(L, n))
	return NULL;
    lua_getfield(L, LUA_REGISTRYINDEX, xstr_mt);
    if (!lua_rawequal(L, -1, -2))
	ud = NULL;		/* wrong kind of userdata */
    lua_pop(L, 2);
    return ud;
}

/* public: return xsbuf_t pointer for the given xstr_t, or NULL if error */
xsbuf_t *xstring_toxsbuf(lua_State *L, int n) {
    xstr_t *xs = _xstr_get(L, n);
    return xs ? xs->buf : NULL;
}

/* public: return data addr and len for the given xstr_t, or NULL if error */
const char *xstring_toaddr(lua_State *L, int n, size_t *l) {
    xstr_t *xs = _xstr_get(L, n);
    if (!xs)
	return NULL;
    if (l)
	*l = xs->len;
    return xs->buf->data + xs->pos;
}

/* public: return data addr and len for the given xstr_t, or throw a type error */
const char *xstring_check(lua_State *L, int n, size_t *l) {
    const char *p = xstring_toaddr(L, n, l);
    if (!p)
	luaL_typerror(L, n, xstr_mt);
    return p;
}


/*----------------------------------------------------------------
 * LUA ACCESSORS
 */

static int xstr_string(lua_State *L) {
    const xstr_t *xs = checkxs(L, 1);
    if (_xstr_valid(xs))
	lua_pushlstring(L, xs->buf->data + xs->pos, xs->len);
    else
	lua_pushnil(L);
    return 1;
}

static int xstr_addr(lua_State *L) {
    const xstr_t *xs = checkxs(L, 1);
    if (_xstr_valid(xs))
	lua_pushlightuserdata(L, (void *)xs->buf->data);
    else
	lua_pushnil(L);
    return 1;
}

static int xstr_size(lua_State *L) {
    const xstr_t *xs = checkxs(L, 1);
    if (_xstr_valid(xs))
	lua_pushinteger(L, xs->len);
    else
	lua_pushnil(L);
    return 1;
}

/*
 * substring implementation
 */

static int xstr_sub(lua_State *L) {
    const xstr_t *xs;
    xstr_t *newxs;
    lua_Integer from, to;

    xs = checkxs(L, 1);
    if (!_xstr_valid(xs))
	goto bad;

    from = luaL_checkint(L, 2);
    if (from < 1 || (size_t)from > xs->len)
	goto bad;

    to = luaL_optint(L, 3, (lua_Integer)xs->len - 1);
    if (to < 0   || (size_t)to   > xs->len)
	goto bad;

    newxs = _xstr_newptr(L, xs->buf);
    if (to >= from) {		/* nonzero length */
	newxs->pos = xs->pos + (from - 1);
	newxs->len = to      - (from - 1);
    }
    else {			/* normalize zero length, just for fun */
	newxs->pos = 0;
	newxs->len = 0;
    }
    return 1;

  bad:
    lua_pushnil(L);
    return 1;
}


/*----------------------------------------------------------------
 * LUA API GLUE
 */

static const luaL_Reg xstr_regf[] = {
    { "__gc",       xstr_gc          },
    { "__tostring", xstr_string      },
    { "string",     xstr_string      },
    { "addr",       xstr_addr        },
    { "size",       xstr_size        },
    { "sub",        xstr_sub         },
    { 0, 0 }
};

int luaopen_xstring(lua_State *L) {
    static const char pubmod[] = "xstring";

    luaL_register(L, pubmod, xstr_regf);

    /* registry.xstring = xstring */
    lua_getglobal(L, pubmod);
    lua_setfield(L, LUA_REGISTRYINDEX, pubmod);

    /* xstring.__index = xstring */
    lua_getglobal(L, pubmod);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    lua_pop(L, 1);

#if 0
    /* if someday there's a separate xstring.core module */
    static const char coremod[] = "xstring.core";
    const char *s;
    if ((s = luaL_findtable(L, LUA_GLOBALSINDEX, coremod, 0)))
        luaL_error(L, "name conflict for module " LUA_QS, coremod);
#endif

    return 1;
}
