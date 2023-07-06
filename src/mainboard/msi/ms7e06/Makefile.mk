## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += msi_id.S

romstage-y += romstage_fsp_params.c

ramstage-y += mainboard.c

all-y += die.c
smm-y += die.c
