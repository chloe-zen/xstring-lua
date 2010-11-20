#!/usr/bin/make

VERSION        = 1.1
PROD           = xstring-$(VERSION)

SO_LINK_PKGS   =
PROG_LINK_PKGS = lua5.1
COMP_PKGS      = $(PROG_LINK_PKGS)

pkg-libs     = $(foreach P,$1,$(shell pkg-config --libs   $P))
pkg-cflags   = $(foreach P,$1,$(shell pkg-config --cflags $P))

OPTIMIZE     = -g -O2
CFLAGS       = -Wall -Wformat $(OPTIMIZE) $(call pkg-cflags, $(COMP_PKGS))
SOFLAGS      = -shared
LDFLAGS      = $(OPTIMIZE)
PROG_LDFLAGS = $(LDFLAGS) -Wl,-rpath,$(shell pwd)
SO_LIBS      = $(call pkg-libs, $(SO_LINK_PKGS))
PROG_LIBS    = $(call pkg-libs, $(PROG_LINK_PKGS)) -lrt

TARGETS      = xstring.so testxs xsperf

all: $(TARGETS)

tardist:
	rm -rf tmp/$(PROD)
	mkdir -p tmp/$(PROD)
	cp -a $(shell awk '{print $$1}' MANIFEST) tmp/$(PROD)
	tar -C tmp -cvzf $(PROD).tar.gz $(PROD)

xstring.so: xstring.o
	$(CC) $(LDFLAGS) $(SOFLAGS) -o $@ $^ $(SO_LIBS)

xstring.o: xstring.c xstring.h

testxs xsperf: %: %.o xstring.so
	$(CC) $(PROG_LDFLAGS) -o $@ $^ $(PROG_LIBS)

clean:
	rm -f $(TARGETS) *.o *.tmp
