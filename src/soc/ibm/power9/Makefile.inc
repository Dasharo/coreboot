## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_CPU_IBM_POWER9),y)

bootblock-y += bootblock.c
bootblock-y += rom_media.c
romstage-y += rom_media.c
romstage-y += romstage.c
romstage-y += mvpd.c
romstage-y += vpd.c
romstage-y += powerbus.c
romstage-y += istep_8_1.c
romstage-y += istep_8_2.c
romstage-y += istep_8_3.c
romstage-y += istep_8_4.c
romstage-y += istep_8_9.c
romstage-y += istep_8_10.c
romstage-y += istep_8_11.c
romstage-y += istep_9_2.c
romstage-y += istep_9_4.c
romstage-y += istep_9_6.c
romstage-y += istep_9_7.c
romstage-y += istep_10_1.c
romstage-y += istep_10_6.c
romstage-y += istep_10_10.c
romstage-y += istep_10_12.c
romstage-y += istep_10_13.c
romstage-y += istep_13_2.c
romstage-y += istep_13_3.c
romstage-y += istep_13_4.c
romstage-y += istep_13_6.c
romstage-y += istep_13_8.c
romstage-y += istep_13_9.c
romstage-y += istep_13_10.c
romstage-y += istep_13_11.c
romstage-y += istep_13_13.c
romstage-y += istep_14_1.c
romstage-y += istep_14_2.c
romstage-y += istep_14_3.c
romstage-y += istep_14_5.c
romstage-y += i2c.c
romstage-y += ccs.c
romstage-y += mcbist.c
romstage-y += timer.c
romstage-y += fsi.c
romstage-y += sbeio.c
romstage-y += xscom.c
ramstage-y += chip.c
ramstage-y += homer.c
ramstage-y += rom_media.c
ramstage-y += timer.c
ramstage-y += istep_18_11.c
ramstage-y += istep_18_12.c
ramstage-y += mvpd.c
ramstage-y += vpd.c
ramstage-y += tor.c
ramstage-y += rs4.c
ramstage-y += powerbus.c
ramstage-y += pstates.c
ramstage-y += xive.c
ramstage-y += int_vectors.S
ramstage-y += i2c.c
ramstage-y += occ.c
ramstage-y += fsi.c
ramstage-y += xscom.c

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

endif
