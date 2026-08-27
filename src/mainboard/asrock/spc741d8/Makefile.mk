## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
romstage-y += romstage.c
romstage-y += util.c
ramstage-y += ramstage.c
ramstage-$(CONFIG_DRIVERS_OPTION_CFR) += cfr.c
ramstage-y += util.c
CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/include
