## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c

romstage-y += gpio.c
romstage-y += romstage_fsp_params.c

ramstage-y += mainboard.c

bootblock-y += die.c
romstage-y += die.c
