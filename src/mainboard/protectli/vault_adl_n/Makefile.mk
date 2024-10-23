## SPDX-License-Identifier: GPL-2.0-only

CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/include

bootblock-y += bootblock.c

romstage-y += variants/$(VARIANT_DIR)/gpio.c
romstage-y += romstage_fsp_params.c

ramstage-y += mainboard.c
ramstage-y += hda_verb.c

all-y += die.c
smm-y += die.c

subdirs-y += variants/$(VARIANT_DIR)
