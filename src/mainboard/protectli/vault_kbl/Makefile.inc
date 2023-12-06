## SPDX-License-Identifier: GPL-2.0-or-later

bootblock-y += bootblock.c

ramstage-$(CONFIG_MAINBOARD_USE_LIBGFXINIT) += gma-mainboard.ads

bootblock-y += die.c
romstage-y += die.c
ramstage-y += die.c
smm-y += die.c
