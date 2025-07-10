# SPDX-License-Identifier: GPL-2.0-or-later

PRG  := smmstore_debug

CC      ?= $(CROSS_COMPILE)gcc
HOSTCC  ?= $(CC)
INSTALL ?= /usr/bin/env install
PREFIX  ?= /usr/local

HOSTCFLAGS ?= $(CFLAGS)
HOSTCFLAGS += -Wall -Wextra -MMD -MP -O3

HOSTLDFLAGS ?= $(LDFLAGS)

# there files are in this directory
SRC := smmstore_debug.c

OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.o.d)

.PHONY: all debug clean install

all: $(PRG)

debug: HOSTCFLAGS += -O0 -g
debug: HOSTLDFLAGS += -g
debug: all

install: $(PRG)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) $^ $(DESTDIR)$(PREFIX)/bin/

clean:
	-$(RM) $(PRG) $(OBJ) $(DEP)

$(PRG): $(OBJ)
	$(HOSTCC) -o $@ $^ $(HOSTLDFLAGS)

%.o: %.c
	$(HOSTCC) $(HOSTCFLAGS) -c -o $@ -MF $@.d $<

-include $(DEP)
