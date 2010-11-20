/* xstring performance test
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
#include <unistd.h>		/* getopt */
#include <time.h>		/* clock_gettime, struct timespec */

#define LARGE   128*1024
#define MEDIUM   10*1024
#define SMALL       1024

#define MINPARAMSIZE 50
#define MAXPARAM     20

typedef enum param_type { PARAM_UNSPEC = -1, PARAM_LUASTRING, PARAM_XSTRING } param_type_t;

static void die(const char *fmt, ...);
static double tsdiff(struct timespec from, struct timespec to);

int main(int argc, char **argv) {
    lua_State *L;
    unsigned i, it;
    int c;
    unsigned long n;
    char *p;

    unsigned iters = 1000;
    param_type_t param_type = PARAM_UNSPEC;
    unsigned num_params = 0;
    typedef struct {
	size_t  size;
	char    *buf;
	xsbuf_t *xsbuf;
    } param_t;
    param_t params[MAXPARAM] = {{ 0 }};
    size_t max_param_size = 0;

    struct timespec real_start, real_end, proc_start, proc_end;
    double totalmem;
    int maxmem;

    while ((c = getopt(argc, argv, "i:" "x:" "LMSs:")) != EOF) {
	switch (c) {
	case 'i':
	    iters = strtoul(optarg, &p, 10);
	    if (*p) die("invalid iter count: -i%s\n", optarg);
	    break;
	case 'x':
	    n = strtoul(optarg, &p, 10);
	    if (*p || n > 1) die("invalid string param type (0/1): -x%s\n", optarg);
	    param_type = (param_type_t)n;
	    break;

	case 'L': n = LARGE;  goto add_param;
	case 'M': n = MEDIUM; goto add_param;
	case 'S': n = SMALL;  goto add_param;
	case 's':
	    n = strtoul(optarg, &p, 0);
	    if (*p == 'k') {
		n *= 1024;
		++p;
	    }
	    else if (*p == 'm') {
		n *= 1024 * 1024;
		++p;
	    }
	    if (*p || n < MINPARAMSIZE)
		die("invalid param size: -s%s\n", optarg);
	add_param:
	    if (num_params >= MAXPARAM)
		die("too many parameters; increase MAXPARAM\n");
	    if (!(p = calloc(n, 1)))
		die("Unable to allocate %lu for parameter %u\n", n, num_params + 1);
	    params[num_params].size = n;
	    params[num_params].buf  = p;
	    ++num_params;
	    if (n > max_param_size)
		max_param_size = n;
	    break;
	}
    }
    if (param_type < 0) die("must specify string param type (-x0 or -x1)\n");
    if (!num_params)    die("must specify at least one parameter size\n");

    printf("Test info:\n"
	   "  %u iterations\n"
	   "  %u %s strings: ",
	   iters,
	   num_params,
	   param_type ? "EXTERNAL" : "LUA");
    for (i = 0; i < num_params; ++i) {
	const size_t s = params[i].size;
	if (s >= 1024 && !(s & 1023))
	    printf(" %zuk", s / 1024);
	else
	    printf(" %zu", s);
    }
    printf("\n");

    /*
     * set up Lua environ
     */

    L = luaL_newstate();
    assert(L);
    luaL_openlibs(L);

    if (luaL_dofile(L, "xsperf.lua"))
	die("Can't load xsperf.lua: %s\n", lua_tostring(L, -1));
    lua_settop(L, 0);

    /*
     * run the test
     */

    srand48(getpid());

    lua_checkstack(L, 2 + num_params);

    if (clock_gettime(CLOCK_REALTIME, &real_start))
	die("Can't get starting real time: %s\n", strerror(errno));
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &proc_start))
	die("Can't get starting process time: %s\n", strerror(errno));

    totalmem = 0;
    maxmem = 0;

    for (it = 0; it < iters; ++it) {
	xsbuf_t *b;
	int mem;

	lua_getglobal(L, "test_iter");

	for (i = 0; i < num_params; ++i) {
	    const param_t *par = &params[i];

	    /* modify all the param strings to defeat hashing */
	    snprintf(par->buf, par->size, "param %2u: %lX", i + 1, (unsigned long)lrand48());

	    switch (param_type) {
	    case PARAM_LUASTRING:
		lua_pushlstring(L, par->buf, par->size);
		break;
	    case PARAM_XSTRING:
		b = xsbuf_new(par->buf, par->size);
		params[i].xsbuf = b;
		xstring_new(L, b);
		break;
	    case PARAM_UNSPEC: ; /* silence gcc warning */
	    }
	}

	if (lua_pcall(L, num_params, 0, 0))
	    die("Call #%u to test_iter() failed: %s\n", it + 1, lua_tostring(L, -1));

	if (param_type == PARAM_XSTRING) {
	    for (i = 0; i < num_params; ++i) {
		xsbuf_kill(   params[i].xsbuf);
		xsbuf_discard(params[i].xsbuf);
	    }
	}

	mem = lua_gc(L, LUA_GCCOUNT, 0);
	totalmem += mem;
	if (mem > maxmem)
	    maxmem = mem;
    }

    lua_gc(L, LUA_GCCOLLECT, 0);

    if (clock_gettime(CLOCK_REALTIME, &real_end))
	die("Can't get ending real time: %s\n", strerror(errno));
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &proc_end))
	die("Can't get ending process time: %s\n", strerror(errno));

    for (i = 0; i < num_params; ++i)
	free(params[i].buf);

    lua_close(L);

    printf("Done!\n");
    printf("User time:   %.3f\n", tsdiff(proc_start, proc_end));
    printf("Clock time:  %.3f\n", tsdiff(real_start, real_end));
    printf("Average mem: %.0f KiB\n", totalmem / iters);
    printf("Maximum mem: %d KiB\n", maxmem);

    return 0;
}

static void die(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

static double tsdiff(struct timespec from, struct timespec to) {
    double n = to.tv_sec - from.tv_sec;
    n += (to.tv_nsec - from.tv_nsec) / 1000000000.0;
    return n;
}
