## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_SOC_IBM_POWER9),y)

bootblock-y += bootblock.c
bootblock-y += rom_media.c

romstage-y += cbmem.c
romstage-y += ccs.c
romstage-y += fsi.c
romstage-y += i2c.c
romstage-y += istep_8_1.c
romstage-y += istep_8_2.c
romstage-y += istep_8_3.c
romstage-y += istep_8_4.c
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
romstage-y += mcbist.c
romstage-y += mvpd.c
romstage-y += powerbus.c
romstage-y += sbeio.c
romstage-y += rom_media.c
romstage-y += romstage.c
romstage-y += timer.c
romstage-y += vpd.c

ramstage-y += cbmem.c
ramstage-y += chip.c
ramstage-y += fsi.c
ramstage-y += homer.c
ramstage-y += i2c.c
ramstage-y += int_vectors.S
ramstage-y += istep_18_11.c
ramstage-y += istep_18_12.c
ramstage-y += mvpd.c
ramstage-y += occ.c
ramstage-y += powerbus.c
ramstage-y += pstates.c
ramstage-y += rom_media.c
ramstage-y += timer.c
ramstage-y += tor.c
ramstage-y += vpd.c
ramstage-y += xive.c

MB_DIR = src/mainboard/$(MAINBOARDDIR)
ONECPU_DTB = 1-cpu.dtb

$(obj)/%.dtb: $(MB_DIR)/%.dts
	dtc -I dts -O dtb -o $@ -i $(MB_DIR) $<

cbfs-files-y += $(ONECPU_DTB)
$(ONECPU_DTB)-file := $(obj)/$(ONECPU_DTB)
$(ONECPU_DTB)-type := raw

ifeq ($(CONFIG_SIGNING_KEYS_DIR),"")
    KEYDIR = $(top)/3rdparty/open-power-signing-utils/test/keys
else
    KEYDIR = $(CONFIG_SIGNING_KEYS_DIR)
endif

PHONY += sign_and_add_ecc
sign_and_add_ecc: $(obj)/coreboot.rom | $(ECCTOOL) $(CREATE_CONTAINER)
ifeq ($(CONFIG_SIGNING_KEYS_DIR),"")
	@printf "    NOTE: signing firmware with test keys\n"
endif
	@printf "    SBSIGN  $(subst $(obj)/,,$<)\n"
	[ -e "$(KEYDIR)/hw_key_a.key" ] || ( echo "error: $(KEYDIR)/hw_key_a.key" is missing; exit 1 )
	[ -e "$(KEYDIR)/hw_key_b.key" ] || ( echo "error: $(KEYDIR)/hw_key_b.key" is missing; exit 1 )
	[ -e "$(KEYDIR)/hw_key_c.key" ] || ( echo "error: $(KEYDIR)/hw_key_c.key" is missing; exit 1 )
	[ -e "$(KEYDIR)/sw_key_p.key" ] || ( echo "error: $(KEYDIR)/sw_key_p.key" is missing; exit 1 )
	$(CREATE_CONTAINER) -a $(KEYDIR)/hw_key_a.key -b $(KEYDIR)/hw_key_b.key -c $(KEYDIR)/hw_key_c.key \
	                    -p $(KEYDIR)/sw_key_p.key --payload $< --imagefile $<.signed
	@printf "    ECC     $(subst $(obj)/,,$<)\n"
	$(ECCTOOL) --inject $<.signed --output $<.signed.ecc --p8
ifeq ($(CONFIG_BOOTBLOCK_IN_SEEPROM),y)
	@printf "    ECC     bootblock\n"
	$(ECCTOOL) --inject $(objcbfs)/bootblock.bin --output $(obj)/bootblock.ecc --p8
else
	@printf "    SBSIGN  bootblock\n"
	$(CREATE_CONTAINER) -a $(KEYDIR)/hw_key_a.key -b $(KEYDIR)/hw_key_b.key -c $(KEYDIR)/hw_key_c.key \
	                    -p $(KEYDIR)/sw_key_p.key --payload $(objcbfs)/bootblock.bin \
	                    --imagefile $(obj)/bootblock.signed
	$(ECCTOOL) --inject $< --output $<.ecc --p8
	@printf "    ECC     bootblock\n"
	dd if=$(obj)/bootblock.signed of=$(obj)/bootblock.signed.pad ibs=25486 conv=sync 2> /dev/null
	$(ECCTOOL) --inject $(obj)/bootblock.signed.pad --output $(obj)/bootblock.signed.ecc --p8
	rm $(obj)/bootblock.signed $(obj)/bootblock.signed.pad
endif # CONFIG_BOOTBLOCK_IN_SEEPROM

files_added:: sign_and_add_ecc

endif
