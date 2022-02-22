## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c

romstage-y += memory.c
romstage-y += romstage.c

ramstage-y += ramstage.c

subdirs-y += variants/baseboard
CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/variants/baseboard/include

subdirs-y += variants/$(VARIANT_DIR)
