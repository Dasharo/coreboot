## SPDX-License-Identifier: GPL-2.0-only
ifeq ($(CONFIG_SOC_INTEL_BAYTRAIL),y)

subdirs-y += romstage
subdirs-y += ../../../cpu/intel/microcode
subdirs-y += ../../../cpu/intel/turbo
subdirs-y += ../../../cpu/intel/common

all-y += tsc_freq.c

bootblock-y += ../../../cpu/intel/car/non-evict/cache_as_ram.S
bootblock-y += ../../../cpu/intel/car/bootblock.c
bootblock-y += ../../../cpu/x86/early_reset.S
bootblock-y += bootblock/bootblock.c

romstage-y += iosf.c
romstage-y += memmap.c
romstage-y += pmutil.c
romstage-y += txei.c

postcar-y += iosf.c
postcar-y += memmap.c

ramstage-y += acpi.c
ramstage-y += chip.c
ramstage-y += cpu.c
ramstage-y += dptf.c
ramstage-y += ehci.c
ramstage-y += emmc.c
ramstage-y += fadt.c
ramstage-y += gfx.c
ramstage-y += gpio.c
ramstage-y += hda.c
ramstage-y += iosf.c
ramstage-y += lpe.c
ramstage-y += lpss.c
ramstage-y += memmap.c
ramstage-y += northcluster.c
ramstage-y += pcie.c
ramstage-y += perf_power.c
ramstage-y += pmutil.c
ramstage-y += ramstage.c
ramstage-y += sata.c
ramstage-y += scc.c
ramstage-y += sd.c
ramstage-y += smm.c
ramstage-y += southcluster.c
ramstage-y += txe.c
ramstage-y += txei.c
ramstage-y += xhci.c
ramstage-$(CONFIG_ELOG) += elog.c
ramstage-$(CONFIG_VGA_ROM_RUN) += int15.c

ifeq ($(CONFIG_HAVE_REFCODE_BLOB),y)
ramstage-y += refcode.c
else
ramstage-y += modphy_table.c refcode_native.c
endif

smm-y += iosf.c
smm-y += pmutil.c
smm-y += smihandler.c
smm-y += tsc_freq.c

# Remove as ramstage gets fleshed out
ramstage-y += placeholders.c

postcar-y += ../../../cpu/intel/car/non-evict/exit_car.S

cpu_microcode_bins += 3rdparty/blobs/soc/intel/baytrail/microcode.bin \
	3rdparty/intel-microcode/intel-ucode/06-37-09

CPPFLAGS_common += -Isrc/soc/intel/baytrail/include

ifeq ($(CONFIG_HAVE_MRC),y)

# Bay Trail MRC is an ELF file. Determine the entry address and first loadable
# section offset in the file. Subtract the offset from the entry address to
# determine the final location.
mrcelfoffset = $(shell $(READELF_x86_32) -S -W $(CONFIG_MRC_FILE) | sed -e 's/\[ /[0/' | awk '$$3 ~ /PROGBITS/ { print "0x"$$5; exit }' )
mrcelfentry = $(shell $(READELF_x86_32) -h -W $(CONFIG_MRC_FILE) | grep 'Entry point address' | awk '{print $$NF }')

# Add memory reference code blob.
cbfs-files-y += mrc.bin
mrc.bin-file := $(call strip_quotes,$(CONFIG_MRC_FILE))
mrc.bin-position := $(shell printf "0x%x" $$(( $(mrcelfentry) - $(mrcelfoffset) )) )
mrc.bin-type := mrc

endif

ifeq ($(CONFIG_TXE_SECURE_BOOT),y)

cbfs-files-y += manifests.bin
manifests.bin-file := $(objcbfs)/sb_manifests
manifests.bin-type := raw

ifeq ($(CONFIG_TXE_SB_INCLUDE_KEY_MANIFEST),y)

manifests.bin-COREBOOT-position := $(call int-subtract, 0x100000000 0x21000)

ifneq ($(call strip_quotes,$(CONFIG_TXE_SB_KEY_MANIFEST_PATH)),)

$(objcbfs)/sb_manifests: $(call strip_quotes,$(CONFIG_TXE_SB_KEY_MANIFEST_PATH))
	dd if=/dev/zero of=$@ bs=5120 count=1 2> /dev/null
	dd if=$< of=$@ conv=notrunc 2> /dev/null
else

$(objcbfs)/sb_manifests:
	dd if=/dev/zero of=$@ bs=5120 count=1 2> /dev/null

endif

else # CONFIG_TXE_SB_INCLUDE_KEY_MANIFEST

manifests.bin-COREBOOT-position := $(call int-subtract, 0x100000000 0x20000)

$(objcbfs)/sb_manifests:
	dd if=/dev/zero of=$@ bs=1024 count=1 2> /dev/null

endif # CONFIG_TXE_SB_INCLUDE_KEY_MANIFEST

files_added:: $(obj)/coreboot.rom $(TXESBMANTOOL)
	if [ "$(CONFIG_TXE_SB_GENERATE_KEY_MANIFEST)" = 'y' ]; then \
		$(TXESBMANTOOL) $(obj)/coreboot.rom generate-km \
			--sbm-key-file $(CONFIG_TXE_SB_SBM_MANIFEST_KEY_PATH) \
			--km-key-file $(CONFIG_TXE_SB_KEY_MANIFEST_KEY_PATH) \
			--km-id $(CONFIG_TXE_SB_KEY_MANIFEST_ID) \
			--svn $(CONFIG_TXE_SB_KEY_MANIFEST_SVN) \
			;\
		printf "    TXE Secure Boot Key Manifest generated.\n"; \
	fi; \
	$(TXESBMANTOOL) $(obj)/coreboot.rom generate-sbm \
		--sbm-key-file $(CONFIG_TXE_SB_SBM_MANIFEST_KEY_PATH) \
		--svn $(CONFIG_TXE_SB_SBM_MANIFEST_SVN) ;\
	printf "    TXE Secure Boot Manifest generated.\n\n"; \

endif # CONFIG_TXE_SECURE_BOOT

endif # CONFIG_SOC_INTEL_BAYTRAIL
