# txesbmantool - Utility for generating TXE Secure Boot manifests
# SPDX-License-Identifier: GPL-2.0-only

CC      ?= gcc
INSTALL ?= /usr/bin/env install
PREFIX  ?= /usr/local

HOSTCC ?= $(CC)
HOSTCFLAGS ?= $(CFLAGS)
top := ../..
objutil := ..

ifneq ($(PKG_CONFIG),)
HOSTPKGCONFIG ?= $(PKG_CONFIG)
else
HOSTPKGCONFIG ?= pkg-config
endif
HOSTCFLAGS += $(shell $(HOSTPKGCONFIG) --cflags wolfssl)
LDFLAGS ?= $(shell $(HOSTPKGCONFIG) --libs wolfssl)

include Makefile.mk

PROGRAM=$(objutil)/txesbmantool/txesbmantool

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) *.o *~ .dependencies
distclean: clean

install: $(PROGRAM)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) $(PROGRAM) $(DESTDIR)$(PREFIX)/bin

.PHONY: all clean distclean install
