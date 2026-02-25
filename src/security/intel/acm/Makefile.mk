## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_INTEL_ACM),y)

ifneq ($(CONFIG_INTEL_TXT_BIOSACM_FILE),"")
cbfs-files-y += $(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)
$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)-file := $(CONFIG_INTEL_TXT_BIOSACM_FILE)
$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)-type := raw
$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)-align := $(CONFIG_INTEL_TXT_BIOSACM_ALIGNMENT)

ifeq ($(CONFIG_INTEL_TOP_SWAP_SEPARATE_REGIONS),y)
regions-for-file-$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM) = BOOTBLOCK,TOPSWAP
endif # INTEL_TOP_SWAP_SEPARATE_REGIONS

ifeq ($(CONFIG_CPU_INTEL_FIRMWARE_INTERFACE_TABLE),y)
$(call add_intermediate, add_acm_fit, $(IFITTOOL) set_fit_ptr)
	$(IFITTOOL) -r $(BB_FIT_REGION) -a -n $(CONFIG_INTEL_TXT_CBFS_BIOS_ACM) -t 2 \
		-s $(CONFIG_CPU_INTEL_NUM_FIT_ENTRIES) -f $<

ifneq ($(CONFIG_INTEL_TOP_SWAP_SEPARATE_REGIONS),y)
$(call add_intermediate, add_acm_ts_fit, $(IFITTOOL) set_fit_ptr)
	$(IFITTOOL) -r TOPSWAP -a -n $(CONFIG_INTEL_TXT_CBFS_BIOS_ACM) -t 2 \
		-s $(CONFIG_CPU_INTEL_NUM_FIT_ENTRIES) -f $<
endif # INTEL_TOP_SWAP_SEPARATE_REGIONS

endif # CPU_INTEL_FIRMWARE_INTERFACE_TABLE

endif # INTEL_TXT_BIOSACM_FILE

ifeq ($(CONFIG_INTEL_TXT_BIOSACM_STUB),y)

BIOS_ACM_STUB := $(obj)/txt_bios_acm_stub.bin

$(BIOS_ACM_STUB): $(obj)/config.h
	dd if=/dev/zero bs=1 count=$(shell printf '%d' $(CONFIG_INTEL_TXT_BIOSACM_STUB_SIZE)) of=$@ 2>/dev/null

cbfs-files-y += $(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)
$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)-file := $(BIOS_ACM_STUB)
$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)-type := raw
$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM)-align := $(CONFIG_INTEL_TXT_BIOSACM_ALIGNMENT)

ifeq ($(CONFIG_INTEL_TOP_SWAP_SEPARATE_REGIONS),y)
regions-for-file-$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM) = BOOTBLOCK,TOPSWAP
bios-acm-stub-regions := BOOTBLOCK TOPSWAP
else
# Without an explicit regions-for-file override the vboot Makefile places
# the file in VBOOT_PARTITIONS: COREBOOT plus every active RW slot.
# Mirror that set here so the stub is removed from every region it lands in.
bios-acm-stub-regions := COREBOOT
ifeq ($(CONFIG_VBOOT_SLOTS_RW_A),y)
bios-acm-stub-regions += FW_MAIN_A
endif
ifeq ($(CONFIG_VBOOT_SLOTS_RW_AB),y)
bios-acm-stub-regions += FW_MAIN_B
endif
endif # INTEL_TOP_SWAP_SEPARATE_REGIONS

$(call add_intermediate, remove_bios_acm_stub, $(CBFSTOOL))
	for r in $(bios-acm-stub-regions); do \
		$(CBFSTOOL) $< remove -r $$r -n $(CONFIG_INTEL_TXT_CBFS_BIOS_ACM) || exit 1; \
	done

endif # INTEL_TXT_BIOSACM_STUB

ifneq ($(CONFIG_INTEL_TXT_SINITACM_FILE),"")
cbfs-files-y += $(CONFIG_INTEL_TXT_CBFS_SINIT_ACM)
$(CONFIG_INTEL_TXT_CBFS_SINIT_ACM)-file := $(CONFIG_INTEL_TXT_SINITACM_FILE)
$(CONFIG_INTEL_TXT_CBFS_SINIT_ACM)-type := raw
$(CONFIG_INTEL_TXT_CBFS_SINIT_ACM)-align := 0x10
$(CONFIG_INTEL_TXT_CBFS_SINIT_ACM)-compression := lzma

ifeq ($(CONFIG_INTEL_TOP_SWAP_SEPARATE_REGIONS),y)
regions-for-file-$(CONFIG_INTEL_TXT_CBFS_SINIT_ACM) = BOOTBLOCK,TOPSWAP
endif # INTEL_TOP_SWAP_SEPARATE_REGIONS

endif

ifeq ($(CONFIG_CPU_INTEL_FIRMWARE_INTERFACE_TABLE),y)

# CBnT does not use FIT for IBB
ifneq ($(CONFIG_INTEL_CBNT_SUPPORT),y)
# Initial BootBlock files
ibb-files := $(foreach file,$(cbfs-files), \
	$(if $(shell echo '$(call extract_nth,7,$(file))'|grep -- --ibb), \
		$(call extract_nth,2,$(file)),))

ibb-files += bootblock

$(call add_intermediate, add_ibb_fit, $(IFITTOOL) set_fit_ptr)
	$(foreach file, $(ibb-files), $(shell $(IFITTOOL) -f $< -a -n $(file) -t 7 \
		-s $(CONFIG_CPU_INTEL_NUM_FIT_ENTRIES) -r COREBOOT)) true

endif # INTEL_CBNT_SUPPORT

endif # CPU_INTEL_FIRMWARE_INTERFACE_TABLE

endif # INTEL_ACM
