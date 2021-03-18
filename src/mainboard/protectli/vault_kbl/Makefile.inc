## SPDX-License-Identifier: GPL-2.0-or-later


ramstage-y += ramstage.c

ramstage-$(CONFIG_MAINBOARD_USE_LIBGFXINIT) += gma-mainboard.ads

subdirs-y += variants/baseboard
CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/variants/baseboard/include

subdirs-y += variants/$(VARIANT_DIR)
CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/variants/$(VARIANT_DIR)/include
