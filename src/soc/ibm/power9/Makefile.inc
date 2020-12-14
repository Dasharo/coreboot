## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_CPU_IBM_POWER9),y)

bootblock-y += bootblock.c
romstage-y += romstage.c
ramstage-y += chip.c
ramstage-y += timer.c

endif
