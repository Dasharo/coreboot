## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c

$(call src-to-obj,bootblock,$(dir)/msi_id.S): $(obj)/fmap_config.h
bootblock-y += msi_id.S

romstage-y += romstage_fsp_params.c

ramstage-y += mainboard.c
ramstage-y += smbios.c

all-y += die.c
smm-y += die.c
