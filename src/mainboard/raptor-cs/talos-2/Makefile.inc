## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
romstage-y += cbmem.c

MB_DIR = src/mainboard/$(MAINBOARDDIR)
ONECPU_DTB = 1-cpu.dtb
TWOCPU_DTB = 2-cpus.dtb

$(obj)/%.dtb: $(MB_DIR)/%.dts
	dtc -I dts -O dtb -o $@ -i $(MB_DIR) $<

$(obj)/$(TWOCPU_DTB): $(obj)/$(ONECPU_DTB)

cbfs-files-y += $(ONECPU_DTB)
$(ONECPU_DTB)-file := $(obj)/$(ONECPU_DTB)
$(ONECPU_DTB)-type := raw

cbfs-files-y += $(TWOCPU_DTB)
$(TWOCPU_DTB)-file := $(obj)/$(TWOCPU_DTB)
$(TWOCPU_DTB)-type := raw
