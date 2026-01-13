## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_INTEL_ACM),y)

ifneq ($(CONFIG_INTEL_TOP_SWAP_SEPARATE_REGIONS),y)
BB_FIT_REGION = COREBOOT
else
BB_FIT_REGION = BOOTBLOCK
endif

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
