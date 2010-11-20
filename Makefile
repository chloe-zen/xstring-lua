#!/usr/bin/make

VERSION        = 1.0

SO_LINK_PKGS   =
PROG_LINK_PKGS = lua5.1
COMP_PKGS      = $(PROG_LINK_PKGS)

pkg-libs     = $(foreach P,$1,$(shell pkg-config --libs   $P))
pkg-cflags   = $(foreach P,$1,$(shell pkg-config --cflags $P))

OPTIMIZE     = -g
CFLAGS       = -Wall -Wformat $(OPTIMIZE) $(call pkg-cflags, $(COMP_PKGS))
SOFLAGS      = -shared
LDFLAGS      = $(OPTIMIZE)
PROG_LDFLAGS = $(LDFLAGS) -Wl,-rpath,$(shell pwd)
SO_LIBS      = $(call pkg-libs, $(SO_LINK_PKGS))
PROG_LIBS    = $(call pkg-libs, $(PROG_LINK_PKGS)) -lrt

TARGETS      = xstring.so testxs xsperf

all: $(TARGETS)

tardist:
	tar -cvzf xstring-$(VERSION).tar.gz $(shell awk '{print $$1}' MANIFEST)

xstring.so: xstring.o
	$(CC) $(LDFLAGS) $(SOFLAGS) -o $@ $^ $(SO_LIBS)

xstring.o: xstring.c xstring.h

testxs xsperf: %: %.o xstring.so
	$(CC) $(PROG_LDFLAGS) -o $@ $^ $(PROG_LIBS)

clean:
	rm -f $(TARGETS) *.o *.tmp
