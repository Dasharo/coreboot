# SPDX-License-Identifier: BSD-3-Clause

txesbmantoolsrc = txesbmantool.c

TXESBMANTOOLCFLAGS  = -Wall -Wextra -Wmissing-prototypes -Werror
TXESBMANTOOLCFLAGS += $(shell $(HOSTPKGCONFIG) --libs wolfssl)

$(objutil)/txesbmantool/txesbmantool: $(addprefix $(top)/util/txesbmantool/,$(txesbmantoolsrc))
	printf "   TXESBMANTOOL\n"
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $< $(TXESBMANTOOLCFLAGS)
