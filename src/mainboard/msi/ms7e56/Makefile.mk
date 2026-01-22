# SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += early_gpio.c

ramstage-y += gpio.c
ramstage-y += update_devicetree.c

ifneq ($(wildcard $(src)/mainboard/$(MAINBOARDDIR)/data*.apcb),)
APCB_SOURCES = $(src)/mainboard/$(MAINBOARDDIR)/data.apcb
APCB_SOURCES_RECOVERY = $(src)/mainboard/$(MAINBOARDDIR)/data_rec.apcb
APCB_SOURCES_68 = $(src)/mainboard/$(MAINBOARDDIR)/data_rec68.apcb
else
show_notices:: warn_no_apcb
endif
