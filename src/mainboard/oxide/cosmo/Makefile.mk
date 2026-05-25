## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c

ifneq ($(wildcard $(src)/mainboard/$(MAINBOARDDIR)/data.apcb),)
ifeq ($(CONFIG_BUILD_WITH_DEBUG_APCB),y)
APCB_SOURCES_68 = $(src)/mainboard/$(MAINBOARDDIR)/data_debug.apcb
else
APCB_SOURCES_68 = $(src)/mainboard/$(MAINBOARDDIR)/data.apcb
endif
else
show_notices:: warn_no_apcb
endif
