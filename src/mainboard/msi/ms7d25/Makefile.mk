## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
$(call src-to-obj,bootblock,$(dir)/msi_id.S): $(obj)/fmap_config.h
bootblock-y += msi_id.S

romstage-y += romstage_fsp_params.c

ramstage-y += mainboard.c
ramstage-y += smbios.c

all-y += die.c
smm-y += die.c

ifeq ($(CONFIG_MSI_ROMHOLE_IN_CBFS),y)

$(obj)/mainboard/$(MAINBOARDDIR)/bpa.bin:
	dd if=/dev/zero of=$@ bs=64K count=1

cbfs-files-y += bpa.bin

bpa.bin-file := $(obj)/mainboard/$(MAINBOARDDIR)/bpa.bin
bpa.bin-type := raw
bpa.bin-compression := none
bpa.bin-position := 0xff080000

endif