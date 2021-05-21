## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_CPU_IBM_POWER9),y)

bootblock-y += bootblock.c
bootblock-y += rom_media.c
romstage-y += rom_media.c
romstage-y += romstage.c
romstage-y += vpd.c
romstage-y += i2c.c
romstage-y += ccs.c
ramstage-y += chip.c
ramstage-y += rom_media.c
ramstage-y += timer.c

endif
