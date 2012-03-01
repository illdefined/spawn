CPPFLAGS ?= -Wall
CFLAGS   ?= -pipe -O2

DESTDIR  ?= /
PREFIX   ?= usr/local

CPPFLAGS += -std=c99 -D_XOPEN_SOURCE=600
LDFLAGS  += -lev

spawn: spawn.c

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^

.c:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $>

bin    := $(DESTDIR)$(PREFIX)/bin/
man    := $(DESTDIR)$(PREFIX)/share/man1/

install: $(bin)spawn $(man)spawn.1

$(bin)spawn: spawn $(bin)
	cp -p $(@F) $(@D)

$(man)spawn.1: spawn.1 $(man)
	cp -p $(@F) $(@D)

$(bin):
	mkdir -p $@

$(man):
	mkdir -p $@

.PHONY: install
