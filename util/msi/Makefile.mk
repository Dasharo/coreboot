TOOLCPPFLAGS += -include $(top)/src/commonlib/bsd/include/commonlib/bsd/compiler.h

MSIROMHOLETOOL:= $(objutil)/msi/romholetool

$(MSIROMHOLETOOL): $(dir)/romholetool/romholetool.c
	printf "    HOSTCC     Creating MSI ROMHOLE tool\n"
	mkdir -p $(objutil)/msi
	$(HOSTCC) $(TOOLCPPFLAGS) $< -o $@

ifeq ($(CONFIG_VENDOR_MSI)$(CONFIG_BOARD_HAS_MSI_ROMHOLE),yy)

ifeq ($(CONFIG_MSI_ROMHOLE_IN_CBFS),y)

$(obj)/mainboard/$(MAINBOARDDIR)/romhole.bin: $(MSIROMHOLETOOL)
	printf "    TOOL       Creating MSI ROMHOLE blob\n"
	$(MSIROMHOLETOOL) -a $(CONFIG_MSI_ROMHOLE_ADDRESS_IN_CBFS) \
			  -s $(CONFIG_MSI_ROMHOLE_SIZE_IN_CBFS) -o $@


cbfs-files-y += msi_romhole.bin

msi_romhole.bin-file := $(obj)/mainboard/$(MAINBOARDDIR)/romhole.bin
msi_romhole.bin-type := raw
msi_romhole.bin-compression := none
msi_romhole.bin-position := $(CONFIG_MSI_ROMHOLE_ADDRESS_IN_CBFS)

else

$(obj)/mainboard/$(MAINBOARDDIR)/romhole.bin: $(MSIROMHOLETOOL) $(obj)/fmap_config.h
	printf "    TOOL       Creating MSI ROMHOLE blob\n"
	$(MSIROMHOLETOOL) -l $(obj)/fmap_config.h -o $@

build_complete:: $(obj)/mainboard/$(MAINBOARDDIR)/romhole.bin
	@printf "    WRITE      MSI ROMHOLE\n"
	$(CBFSTOOL) $(obj)/coreboot.rom write -u -i 255 -r ROMHOLE -f $<

endif

endif
