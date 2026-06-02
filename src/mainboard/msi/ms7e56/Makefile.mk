# SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += early_gpio.c

ramstage-y += gpio.c
ramstage-y += smbios.c

ifneq ($(wildcard $(src)/mainboard/$(MAINBOARDDIR)/data*.apcb),)
APCB_SOURCES = $(src)/mainboard/$(MAINBOARDDIR)/data.apcb
APCB_SOURCES_RECOVERY = $(src)/mainboard/$(MAINBOARDDIR)/data_rec.apcb
APCB_SOURCES_68 = $(src)/mainboard/$(MAINBOARDDIR)/data_rec68.apcb
else
show_notices:: warn_no_apcb
endif

$(call src-to-obj,bootblock,$(dir)/msi_id.S): $(obj)/fmap_config.h $(obj)/build.h

bootblock-y += msi_id.S

$(obj)/msi_id.bin: $(obj)/bootblock/mainboard/$(MAINBOARDDIR)/msi_id.o
	$(OBJCOPY_bootblock) -O binary $< $@

# The MSI ID must be in the last sectors of the image for the MSI FlashBIOS to
# detect it
$(call add_intermediate, add_msi_id, $(obj)/msi_id.bin)
	@printf "    WRITE MSI_ID\n"
	$(CBFSTOOL) $< write -u -i 255 -r MSI_ID -f $(obj)/msi_id.bin
	rm $(obj)/msi_id.bin
