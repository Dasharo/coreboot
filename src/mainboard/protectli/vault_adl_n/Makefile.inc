## SPDX-License-Identifier: GPL-2.0-only

CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/include

bootblock-y += variants/$(VARIANT_DIR)/bootblock.c

romstage-y += variants/$(VARIANT_DIR)/gpio.c
romstage-y += variants/$(VARIANT_DIR)/romstage_fsp_params.c

ramstage-y += variants/$(VARIANT_DIR)/mainboard.c
ramstage-y += variants/$(VARIANT_DIR)/hda_verb.c

bootblock-y += die.c
romstage-y += die.c

subdirs-y += variants/$(VARIANT_DIR)
