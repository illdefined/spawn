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

install: $(bin)spawn

$(bin)spawn: spawn $(bin)
	cp -p $(@F) $(@D)

$(bin):
	mkdir -p $@

.PHONY: install
