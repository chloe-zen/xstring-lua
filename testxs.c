/* xstring correctness test
 * by Chip Salzenberg <chip@pobox.com> */

#include "xstring.h"

#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/* this bit is Unix-specific */
#include <unistd.h>		/* getopt, write */

static const char
    wool[] = "Pull the wool over your own eyes!";

static void die(const char *fmt, ...);
static void register_funcs(lua_State *L);

int main(int argc, char **argv) {
    lua_State *L;
    xsbuf_t *buf;

    /*
     * set up Lua environ
     */

    L = luaL_newstate();
    assert(L);
    luaL_openlibs(L);

    /*
     * load test harness; this also loads the xstring module;
     * and register the test-specific functions
     */

    if (luaL_dofile(L, "testxs.lua"))
	die("Can't load testxs.lua: %s\n", lua_tostring(L, -1));
    lua_settop(L, 0);
    register_funcs(L);

    /*
     * call test entry point with a few preconstructed xstrings;
     *  if it fails, we do too
     */

    lua_getglobal(L, "testxs");

    buf = xsbuf_new(wool, strlen(wool));
    xstring_new(L, buf);
    xsbuf_discard(buf);		 /* this string is always valid */

    if (lua_pcall(L, 1, 0, 0))
	die("self-test failed: %s\n", lua_tostring(L, -1));

    printf("self-test OK\n");

    /*
     * outahere
     */

    lua_close(L);

    return 0;
}

static void die(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

/*
 * example xstring test functions
 */

static int testxs_xfindbyte(lua_State *L) {
    size_t haystack_len, needle_len;
    const char *haystack = xstring_check(    L, 1, &haystack_len);
    const char *needle   = luaL_checklstring(L, 2, &needle_len  );
    const char *p;

    if (needle_len == 1 && (p = memchr(haystack, needle[0], haystack_len)))
	lua_pushinteger(L, (p - haystack) + 1);
    else
	lua_pushnil(L);
    return 1;
}

static const luaL_Reg testxs_regf[] = {
    { "xfindbyte",    testxs_xfindbyte    },
    { 0, 0 }
};

static void register_funcs(lua_State *L) {
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, testxs_regf);
    lua_pop(L, 1);
}


