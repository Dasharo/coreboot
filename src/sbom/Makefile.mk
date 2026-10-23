## SPDX-License-Identifier: GPL-2.0-only

obj ?= build
src ?= src
build-dir = $(obj)/sbom
src-dir = $(src)/sbom

# Strip quotes from binary paths and SBOM file paths. Each binary path should have a
# corresponding SBOM file path, but not every SBOM file path needs a binary path. That
# is because binary files are only needed if they are used to extract information from
# them which in turn can be included in the SBOM files (like version or config stuff).
# But for some Software there are only SBOM files, which basically tell the most generic
# information about that piece of Software. Ideally one would not need the binary files
# at all, because extacting information out of mostly unknown binary blobs is a pain.
CONFIG_ME_BIN_PATH         := $(call strip_quotes, $(CONFIG_ME_BIN_PATH))
CONFIG_SBOM_ME_PATH        := $(call strip_quotes, $(CONFIG_SBOM_ME_PATH))
CONFIG_FSP_S_FILE               := $(call strip_quotes, $(CONFIG_FSP_S_FILE))
CONFIG_FSP_M_FILE               := $(call strip_quotes, $(CONFIG_FSP_M_FILE))
CONFIG_FSP_T_FILE               := $(call strip_quotes, $(CONFIG_FSP_T_FILE))
CONFIG_FSP_FD_PATH              := $(call strip_quotes, $(CONFIG_FSP_FD_PATH))
CONFIG_SBOM_INTEL_FSP_PATH      := $(call strip_quotes, $(CONFIG_SBOM_INTEL_FSP_PATH))
CONFIG_AGESA_BINARY_PI_FILE     := $(call strip_quotes, $(CONFIG_AGESA_BINARY_PI_FILE))
CONFIG_SBOM_AGESA_PATH          := $(call strip_quotes, $(CONFIG_SBOM_AGESA_PATH))
CONFIG_AMD_OPENSIL_PATH         := $(call strip_quotes, $(CONFIG_AMD_OPENSIL_PATH))
CONFIG_SBOM_OPENSIL_PATH        := $(call strip_quotes, $(CONFIG_SBOM_OPENSIL_PATH))
CONFIG_PAYLOAD_FILE        := $(call strip_quotes, $(CONFIG_PAYLOAD_FILE))
CONFIG_SBOM_PAYLOAD_PATH   := $(call strip_quotes, $(CONFIG_SBOM_PAYLOAD_PATH))
CONFIG_EC_PATH             := $(call strip_quotes, $(CONFIG_EC_PATH))
CONFIG_SBOM_EC_PATH        := $(call strip_quotes, $(CONFIG_SBOM_EC_PATH))
CONFIG_SBOM_EC_BIN_PATH    := $(call strip_quotes, $(CONFIG_SBOM_EC_BIN_PATH))
CONFIG_IFD_BIN_PATH              := $(call strip_quotes, $(CONFIG_IFD_BIN_PATH))
CONFIG_SBOM_IFD_PATH             := $(call strip_quotes, $(CONFIG_SBOM_IFD_PATH))
CONFIG_SBOM_BIOS_ACM_PATH        := $(call strip_quotes, $(CONFIG_SBOM_BIOS_ACM_PATH))
CONFIG_SBOM_SINIT_ACM_PATH       := $(call strip_quotes, $(CONFIG_SBOM_SINIT_ACM_PATH))
CONFIG_INTEL_TXT_CBFS_BIOS_ACM   := $(call strip_quotes, $(CONFIG_INTEL_TXT_CBFS_BIOS_ACM))
CONFIG_INTEL_TXT_CBFS_SINIT_ACM  := $(call strip_quotes, $(CONFIG_INTEL_TXT_CBFS_SINIT_ACM))
# Other subsystems test these options against "" (e.g. src/security/intel/acm),
# so keep them intact and use local copies without quotes.
sbom-bios-acm-file               := $(call strip_quotes, $(CONFIG_INTEL_TXT_BIOSACM_FILE))
sbom-sinit-acm-file              := $(call strip_quotes, $(CONFIG_INTEL_TXT_SINITACM_FILE))
CONFIG_SBOM_ACM_PRODUCT_FAMILY   := $(call strip_quotes, $(CONFIG_SBOM_ACM_PRODUCT_FAMILY))
CONFIG_SBOM_COMPILER_PATH  := $(call strip_quotes, $(CONFIG_SBOM_COMPILER_PATH))
CONFIG_EDK2_REPOSITORY     := $(call strip_quotes, $(CONFIG_EDK2_REPOSITORY))
CONFIG_SBOM_EDK2_PLATFORMS_PATH := $(call strip_quotes, $(CONFIG_SBOM_EDK2_PLATFORMS_PATH))
CONFIG_VGA_BIOS_FILE            := $(call strip_quotes, $(CONFIG_VGA_BIOS_FILE))
CONFIG_VGA_BIOS_ID              := $(call strip_quotes, $(CONFIG_VGA_BIOS_ID))
CONFIG_VGA_BIOS_SECOND_FILE     := $(call strip_quotes, $(CONFIG_VGA_BIOS_SECOND_FILE))
CONFIG_VGA_BIOS_SECOND_ID       := $(call strip_quotes, $(CONFIG_VGA_BIOS_SECOND_ID))
CONFIG_VGA_BIOS_DGPU_FILE       := $(call strip_quotes, $(CONFIG_VGA_BIOS_DGPU_FILE))
CONFIG_VGA_BIOS_DGPU_ID         := $(call strip_quotes, $(CONFIG_VGA_BIOS_DGPU_ID))
CONFIG_EDK2_GOP_FILE            := $(call strip_quotes, $(CONFIG_EDK2_GOP_FILE))
CONFIG_EDK2_LAN_ROM_DRIVER      := $(call strip_quotes, $(CONFIG_EDK2_LAN_ROM_DRIVER))
CONFIG_SBOM_VGA_BIOS_PATH        := $(call strip_quotes, $(CONFIG_SBOM_VGA_BIOS_PATH))
CONFIG_SBOM_VGA_BIOS_SECOND_PATH := $(call strip_quotes, $(CONFIG_SBOM_VGA_BIOS_SECOND_PATH))
CONFIG_SBOM_VGA_BIOS_DGPU_PATH   := $(call strip_quotes, $(CONFIG_SBOM_VGA_BIOS_DGPU_PATH))
CONFIG_SBOM_EDK2_GOP_PATH        := $(call strip_quotes, $(CONFIG_SBOM_EDK2_GOP_PATH))
CONFIG_SBOM_EDK2_LAN_ROM_PATH    := $(call strip_quotes, $(CONFIG_SBOM_EDK2_LAN_ROM_PATH))
CONFIG_SBOM_IPXE_PATH      := $(call strip_quotes, $(CONFIG_SBOM_IPXE_PATH))
CONFIG_SBOM_STM_PATH       := $(call strip_quotes, $(CONFIG_SBOM_STM_PATH))
sbom-stm-file              := $(call strip_quotes, $(CONFIG_STM_BINARY_FILE))
CONFIG_SBOM_AMD_BLOB_LICENSE    := $(call strip_quotes, $(CONFIG_SBOM_AMD_BLOB_LICENSE))
CONFIG_LINUXBOOT_KERNEL_VERSION := $(call strip_quotes, $(CONFIG_LINUXBOOT_KERNEL_VERSION))
CONFIG_LINUXBOOT_UROOT_VERSION  := $(call strip_quotes, $(CONFIG_LINUXBOOT_UROOT_VERSION))
CONFIG_PAYLOAD_FILE        := $(call strip_quotes, $(CONFIG_PAYLOAD_FILE))
CONFIG_SBOM_MANUFACTURER   := $(call strip_quotes, $(CONFIG_SBOM_MANUFACTURER))

# Select the correct payload directory for the used payload. Ideally we could just make this
# a one-liner, but since the payload is generated externally (with an extra make command), we
# have to hard code the paths here.
ifeq ($(CONFIG_SBOM_PAYLOAD_GENERATE), y)
payload-git-dir-$(CONFIG_PAYLOAD_BOOTBOOT)    = payloads/external/BOOTBOOT/bootboot
payload-git-dir-$(CONFIG_PAYLOAD_DEPTHCHARGE) = payloads/external/depthcharge/depthcharge
payload-git-dir-$(CONFIG_PAYLOAD_FILO)        = payloads/external/FILO/filo
payload-git-dir-$(CONFIG_PAYLOAD_GRUB2)       = payloads/external/GRUB2/grub2
payload-git-dir-$(CONFIG_PAYLOAD_LINUXBOOT)   = payloads/external/LinuxBoot/linuxboot
payload-git-dir-$(CONFIG_PAYLOAD_SEABIOS)     = payloads/external/SeaBIOS/seabios
payload-git-dir-$(CONFIG_PAYLOAD_SKIBOOT)     = payloads/external/skiboot/skiboot
payload-git-dir-$(CONFIG_PAYLOAD_UBOOT)       = payloads/external/U-Boot/u-boot
payload-git-dir-$(CONFIG_PAYLOAD_IPXE)        = payloads/external/iPXE/ipxe
# edk2 workspace is nested: payloads/external/edk2/workspace/<org>/
# The repo directory name is word 3 of the repository URL (after splitting on '/'),
# matching the logic in payloads/external/edk2/Makefile.
payload-git-dir-$(CONFIG_PAYLOAD_EDK2)        = payloads/external/edk2/workspace/$(word 3,$(subst /, ,$(CONFIG_EDK2_REPOSITORY)))
ifneq ($(payload-git-dir-y),)
# W/A for multijob build:
# Rule to ensure the payload repo is cloned, if not available at the time of SBOM creation
$(payload-git-dir-y)/.git: $(CONFIG_PAYLOAD_FILE)
# only proceed with payload sbom data, if one of the above payloads were selected (should be guarded by Kconfig as well)
# e.g. payload-git-dir-y=payloads/external/SeaBIOS/seabios -> payload-json-file=$(build-dir)/payload-SeaBIOS.json
ifeq ($(CONFIG_PAYLOAD_EDK2), y)
payload-swid          = $(build-dir)/payload-edk2.json
payload-swid-template = $(src-dir)/payload-edk2.json
else
payload-swid = $(build-dir)/payload-$(subst /,,$(dir $(patsubst payloads/external/%,%,$(payload-git-dir-y)))).json
payload-swid-template = $(patsubst $(build-dir)/%.json,$(src-dir)/%.json,$(payload-swid))
endif # ifneq ($(payload-git-dir-y),)
endif
endif # ifeq ($(CONFIG_SBOM_PAYLOAD_GENERATE), y)


# Keep standalone "make sbom" rebuilds read-only with respect to payloads:
# use already checked-out repositories for version extraction and avoid
# re-triggering payload fetch/build targets (especially with `make -B sbom`).
ifeq ($(filter sbom,$(MAKECMDGOALS)),sbom)
payload-swid-ready-dep := $(wildcard $(payload-git-dir-y)/.git)
ipxe-swid-ready-dep := $(wildcard payloads/external/iPXE/ipxe/.git)
edk2-platforms-swid-ready-dep := $(wildcard payloads/external/edk2/workspace/edk2-platforms/.git)
stm-swid-ready-dep := $(wildcard $(sbom-stm-file))
else
payload-swid-ready-dep := $(CONFIG_PAYLOAD_FILE)
ipxe-swid-ready-dep := payloads/external/iPXE/ipxe/ipxe.rom
# edk2-platforms is checked out as part of the edk2 payload build, so gate its
# SBOM extraction on the built payload file being ready.
edk2-platforms-swid-ready-dep := $(CONFIG_PAYLOAD_FILE)
stm-swid-ready-dep := $(sbom-stm-file)
endif

# Add all SBOM files into the swid-files-y target. This target contains all
# .json, .ini, .uswid, .xml, .pc SBOM files that are later merged into one uSWID SBOM file.
# Some of these have an option that this Makefile generates/extracts some information from
# binary files in order to give more complete/detailed information inside the SBOM file.
# These files are either in src/sbom/ or build/sbom (if they are generated).
swid-files-$(CONFIG_SBOM_ME) += $(if $(CONFIG_SBOM_ME_GENERATE), $(build-dir)/intel-me.json, $(CONFIG_SBOM_ME_PATH))
# ME/TXE on IFWI platforms (Apollo Lake/Gemini Lake): SBOM_ME cannot be
# enabled there through Kconfig, because it depends on HAVE_ME_BIN and the
# TXE is stitched inside the IFWI image instead of being a standalone
# binary. Include the generated ME/TXE tag whenever an SBOM is built on
# such platforms (same approach as the edk2 payload override above).
ifeq ($(CONFIG_NEED_IFWI),y)
ifneq ($(CONFIG_SBOM_ME),y)
swid-files-y += $(build-dir)/intel-me.json
endif
endif
swid-files-$(CONFIG_SBOM_PAYLOAD) += $(if $(CONFIG_SBOM_PAYLOAD_GENERATE),$(payload-swid),$(if $(CONFIG_PAYLOAD_EDK2),$(payload-swid),$(CONFIG_SBOM_PAYLOAD_PATH)))
swid-files-$(CONFIG_SBOM_EDK2_PLATFORMS) += $(if $(CONFIG_SBOM_EDK2_PLATFORMS_GENERATE),$(build-dir)/payload-edk2-platforms.json,$(CONFIG_SBOM_EDK2_PLATFORMS_PATH))
swid-files-$(CONFIG_SBOM_INTEL_MICROCODE) += $(foreach ucode,$(filter 3rdparty/intel-microcode/intel-ucode/%, $(cpu_microcode_bins)),$(call intel-ucode-swids,$(ucode)))
swid-files-$(CONFIG_SBOM_AMD_MICROCODE) += $(foreach ucode,$(amd_microcode_bins),$(build-dir)/amd-microcode-$(basename $(notdir $(ucode))).json)
swid-files-$(CONFIG_SBOM_INTEL_FSP) += $(if $(CONFIG_SBOM_INTEL_FSP_GENERATE), $(build-dir)/intel-fsp.json, $(CONFIG_SBOM_INTEL_FSP_PATH))
swid-files-$(CONFIG_SBOM_AGESA) += $(if $(CONFIG_SBOM_AGESA_GENERATE), $(build-dir)/amd-agesa.json, $(CONFIG_SBOM_AGESA_PATH))
swid-files-$(CONFIG_SBOM_OPENSIL) += $(if $(CONFIG_SBOM_OPENSIL_GENERATE), $(build-dir)/amd-opensil.json, $(CONFIG_SBOM_OPENSIL_PATH))
swid-files-$(CONFIG_SBOM_IFD) += $(if $(CONFIG_SBOM_IFD_GENERATE),$(build-dir)/intel-ifd.json,$(CONFIG_SBOM_IFD_PATH))
swid-files-$(CONFIG_SBOM_EC) += $(if $(CONFIG_SBOM_EC_GENERATE),$(build-dir)/generic-ec.json,$(CONFIG_SBOM_EC_PATH))
# For ACM entries: when the build provides the ACM (INTEL_TXT_BIOSACM_FILE,
# INTEL_TXT_SINITACM_FILE), take its version and hash from that file.
# Otherwise extract them from the ROM after provisioning (make sbom).
# The wildcard guard avoids a circular dependency during the initial 'make all'
# (coreboot.rom → sbom.uswid → acm.json → coreboot.rom): when neither the ACM
# file nor the ROM exists, swid-files is left empty for that ACM so the initial
# SBOM builds without its entry.  After post-build ACM provisioning, run
# 'make sbom' to regenerate sbom.uswid with the ACMs extracted from the ROM.
swid-files-$(CONFIG_SBOM_BIOS_ACM) += \
	$(if $(CONFIG_SBOM_BIOS_ACM_GENERATE), \
		$(if $(sbom-bios-acm-file)$(wildcard $(obj)/coreboot.rom),$(build-dir)/intel-bios-acm.json,), \
		$(CONFIG_SBOM_BIOS_ACM_PATH))
swid-files-$(CONFIG_SBOM_SINIT_ACM) += \
	$(if $(CONFIG_SBOM_SINIT_ACM_GENERATE), \
		$(if $(sbom-sinit-acm-file)$(wildcard $(obj)/coreboot.rom),$(build-dir)/intel-sinit-acm.json,), \
		$(CONFIG_SBOM_SINIT_ACM_PATH))

swid-files-$(CONFIG_SBOM_IPXE) += $(if $(CONFIG_SBOM_IPXE_GENERATE),$(build-dir)/payload-iPXE.json,$(CONFIG_SBOM_IPXE_PATH))
swid-files-$(CONFIG_SBOM_STM) += $(if $(CONFIG_SBOM_STM_GENERATE),$(build-dir)/stm.json,$(CONFIG_SBOM_STM_PATH))
# Proprietary graphics/network binary blobs (VGA BIOS OptionROMs, edk2 GOP and
# LAN drivers).  In GENERATE mode a minimal template + blob hash is emitted;
# vendors should prefer supplying a full SBOM via the matching _PATH option.
swid-files-$(CONFIG_SBOM_VGA_BIOS) += $(if $(CONFIG_SBOM_VGA_BIOS_GENERATE),$(build-dir)/vga-bios.json,$(CONFIG_SBOM_VGA_BIOS_PATH))
swid-files-$(CONFIG_SBOM_VGA_BIOS_SECOND) += $(if $(CONFIG_SBOM_VGA_BIOS_SECOND_GENERATE),$(build-dir)/vga-bios-second.json,$(CONFIG_SBOM_VGA_BIOS_SECOND_PATH))
swid-files-$(CONFIG_SBOM_VGA_BIOS_DGPU) += $(if $(CONFIG_SBOM_VGA_BIOS_DGPU_GENERATE),$(build-dir)/vga-bios-dgpu.json,$(CONFIG_SBOM_VGA_BIOS_DGPU_PATH))
swid-files-$(CONFIG_SBOM_EDK2_GOP) += $(if $(CONFIG_SBOM_EDK2_GOP_GENERATE),$(build-dir)/edk2-gop.json,$(CONFIG_SBOM_EDK2_GOP_PATH))
swid-files-$(CONFIG_SBOM_EDK2_LAN_ROM) += $(if $(CONFIG_SBOM_EDK2_LAN_ROM_GENERATE),$(build-dir)/edk2-lan-rom.json,$(CONFIG_SBOM_EDK2_LAN_ROM_PATH))

# AMD PSP FW SBOM files are generated by amdfwtool into build/sbom/amdfw.
# We extend the swid-files-y with JSON files produced by amdfwtool once
# the tool has run.
ifeq ($(CONFIG_SBOM_AMD_PSP_FW),y)

amdfw-sbom-dep := $(obj)/amdfw.rom

define amdfw_swid_get_files
swid-files-$(CONFIG_SBOM_AMD_PSP_FW) += $$(wildcard $$(build-dir)/amdfw/amd-pspfw-*.json)
endef

else

amdfw-sbom-dep :=

define amdfw_swid_get_files
endef

endif

vboot-pkgconfig-files = $(obj)/external/vboot_reference-bootblock/vboot_host.pc $(obj)/external/vboot_reference-ramstage/vboot_host.pc $(obj)/external/vboot_reference-postcar/vboot_host.pc
ifeq ($(CONFIG_SEPARATE_ROMSTAGE),y)
vboot-pkgconfig-files += $(obj)/external/vboot_reference-romstage/vboot_host.pc
endif
swid-files-$(CONFIG_SBOM_VBOOT) += $(if $(CONFIG_SBOM_VBOOT_GENERATE),$(build-dir)/vboot.json,$(vboot-pkgconfig-files))
$(vboot-pkgconfig-files): $(VBOOT_LIB_bootblock) $(VBOOT_LIB_romstage) $(VBOOT_LIB_ramstage) $(VBOOT_LIB_postcar) # src/security/vboot/Makefile.mk

ifeq ($(CONFIG_SBOM_COMPILER),y)
compiler-toolchain = $(CC_bootblock) $(CC_romstage) $(CC_ramstage) $(CC_postcar) $(CC_verstage) $(LD_bootblock) $(LD_romstage) $(LD_ramstage) $(LD_postcar) $(LD_verstage) $(AS_bootblock) $(AS_romstage) $(AS_ramstage) $(AS_postcar) $(AS_verstage)
swid-files-compiler = $(CONFIG_SBOM_COMPILER_PATH)
endif

# include all licenses used in coreboot. Ideally we would only include the licenses,
# which are used in this build
coreboot-licenses = $(foreach license, $(patsubst %.txt, %, $(filter-out retained-copyrights.txt, $(patsubst LICENSES/%, %, $(wildcard LICENSES/*)))), https://spdx.org/licenses/$(license).html)

# only include CBFS SBOM section if there is any data for it
ifeq ($(CONFIG_SBOM),y)
# This captures newline in a variable for use as a separator of recipe lines.
define newline


endef
# Use intermediate so that $(build-dir)/sbom.uswid will be evaluated
# after all files are built, so all repos should be available
$(call add_intermediate, add_sbom_cbfs, $(CBFSTOOL) $(build-dir)/sbom.uswid)
	$(foreach region,$(all-regions), \
		$(if $(CONFIG_UPDATE_IMAGE),-$(CBFSTOOL) $< remove -n sbom 2>/dev/null) \
		$(CBFSTOOL) $< add -r $(region) -n sbom -t raw -f $(build-dir)/sbom.uswid $(newline))
endif

## Build final SBOM (Software Bill of Materials) file in uswid format

$(build-dir)/sbom.uswid: $(build-dir)/coreboot.json $$(swid-files-y) $(swid-files-compiler) | $(build-dir)/goswid $(build-dir) $(amdfw-sbom-dep)
	echo "    SBOM      " $^
	$(eval $(call amdfw_swid_get_files))
	$(build-dir)/goswid convert -o $@ \
		--parent $(build-dir)/coreboot.json \
		$(if $(swid-files-y), --requires $$(echo $(swid-files-y) | tr ' ' ','),) \
		$(if $(swid-files-compiler), --compiler $(swid-files-compiler),)

# all build files depend on the $(build-dir) directory being created
$(build-dir):
	mkdir -p $(build-dir)

$(build-dir)/goswid: | $(build-dir)
	echo "    SBOM      building goswid tool"
	cd util/goswid; \
	GOPATH=$(abspath build/go) GO111MODULE=on go build -modcacherw -o $(abspath $@) ./cmd/goswid

## Generate all .json files

$(build-dir)/compiler-%.json: $(src-dir)/compiler-%.json | $(build-dir)/goswid
	cp $< $@
	ver=$$($(CC_bootblock) --version 2>&1 | head -n 1 | grep -Eo '[0-9]+\.[0-9]+(\.[0-9]+)*' | head -n 1); \
	if [ -n "$$ver" ]; then \
		sed -i "s/<software_version>/$$ver/" $@; \
	else \
		sed -i "/software-version/d" $@; \
	fi
	for tool in $$(echo $(compiler-toolchain) | tr ' ' '\n' | grep -v '^-' | sort | uniq); do \
		command -v "$$tool" > /dev/null 2>&1 || continue; \
		name=$$(basename "$$tool"); \
		version=$$($$tool --version 2>&1 | head -n 1 | grep -Eo '([0-9]+\.[0-9]+\.*[0-9]*)'); \
		[ -n "$$name" ] && [ -n "$$version" ] || continue; \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ --name "$$name" --version "$$version"; \
	done

coreboot-gitdir := $(shell git rev-parse --git-dir)
# coreboot version, format like "<commit-date>_<hash>"
# colloquial_version is latest coreboot release. Assumes static release tag format, might break.
$(build-dir)/coreboot.json: $(src-dir)/coreboot.json $(coreboot-gitdir)/HEAD | $(build-dir)/goswid
	cp $< $@; \
	git_comm_hash=$$(git log -n 1 --format="%cs_%H"); \
	git_latest_rel=$$(git tag --merged HEAD | grep -E '^[0-9]{1,2}\.[0-9]{1,2}$$' | sort -V | tail -n1); \
	sed -i -e "s/<software_version>/$$git_comm_hash/" \
		-e "s/<colloquial_version>/$$git_latest_rel/" \
		$@;\
	$(build-dir)/goswid add-license -o $@ -i $@ $(coreboot-licenses)
	if [ -n "$(CONFIG_SBOM_MANUFACTURER)" ]; then \
		sed -i 's#"entity":[[:space:]]*\[#"entity":[ { "entity-name": "$(CONFIG_SBOM_MANUFACTURER)", "role": [ "softwareCreator", "maintainer" ] },#' $@; \
	fi
	# CRA Art. 13(6)/13(7) link injection removed: rel=advisories and
	# rel=security-contact are not IANA-registered CoSWID link relationships,
	# and upstream python-uswid (used by LVFS) crashes with KeyError on them
	# because uSwidLinkRel.from_string() does a bare dict lookup with no
	# UNKNOWN fallback. The same CVD/security-contact facts are carried by the
	# CRA sidecar instead, so the CoSWID stays LVFS-ingestable. Re-enable only
	# once uswid tolerates unknown rels (or a standard rel exists).

# Extract ME/TXE/SPS version from the firmware binary.
#
# Intel SPS (Server Platform Services) images are handled first, because none
# of the client paths below work on them: they carry no ASCII version string,
# they have no NFTP partition, and their $FPT does not sit at one of the two
# offsets cse_fpt probes, so cse_fpt bails out on the whole image. What they do
# have - and what client CSME/TXE images never have - is an OPR (Operational)
# code partition. Its $CPD header is directly followed by the partition
# manifest, and the version sits 8 bytes after the manifest's $MN2 magic, same
# as for NFTP below. Presence of OPR therefore serves both to detect SPS and to
# pick the SPS CoSWID template, so the tag is not mislabelled as client ME.
#
# Client CSME/TXE: some versions store the version as an ASCII string like:
# "ME16.1.40.2765".
# When the string is missing, try to extract it from the CSE Main program
# (NFTP) partition manifest: the version is 4x2 byte LE fields, 8 bytes after
# the $MN2 magic string.
# How NFTP is located depends on the image layout:
# 1. When NEED_IFWI & CONFIG_IFWI_FILE_NAME,
#    => image has no BPDT, NFTP is located in the IFWI image, extract NFTP from
#       CONFIG_IFWI_FILE_NAME with ifwitool
# 2. CONFIG_SOC_INTEL_CSE_HAVE_SPEC_SUPPORT=y, image has BPDT, ME_SPEC versioned
#    => placed at the last non-empty BPDT partition, extract with cse_serger
#    1. CONFIG_ME_SPEC >= 15 => version is 1.7
#    2. 15 > CONFIG_ME_SPEC >= 12 => version is 1.6
#    3. 12 > CONFIG_ME_SPEC => not possible in such case
#       CONFIG_SOC_INTEL_CSE_HAVE_SPEC_SUPPORT must be "=n"
# 3. CONFIG_SOC_INTEL_CSE_HAVE_SPEC_SUPPORT=n
#    => ME_SPEC <= 11, no BPDT, use cse_fpt to extract NFTP
ifeq ($(CONFIG_NEED_IFWI),y)
    sbom-me-bin := $(call strip_quotes,$(CONFIG_IFWI_FILE_NAME))
else
    sbom-me-bin := $(CONFIG_ME_BIN_PATH)
endif

$(build-dir)/intel-me.json: $(src-dir)/intel-me.json $(src-dir)/intel-sps.json $(sbom-me-bin) | $(build-dir) $(build-dir)/goswid $(IFWITOOL) $(CSE_SERGER) $(CSE_FPT)
	me='$(sbom-me-bin)'; \
	me_ver=""; \
	for cpd in $$(grep -aboF '$$CPD' "$$me" | cut -d: -f1); do \
		name=$$(dd if="$$me" bs=1 skip=$$((cpd + 12)) count=4 2>/dev/null \
			| tr -dc 'A-Za-z0-9'); \
		case "$$name" in OPR*) ;; *) continue ;; esac; \
		man=$$(grep -aboF '$$MN2' "$$me" | cut -d: -f1 \
			| awk -v c="$$cpd" '$$1 >= c { print; exit }'); \
		[ -n "$$man" ] || continue; \
		me_ver=$$(dd if="$$me" skip=$$((man + 8)) bs=1 count=8 2>/dev/null \
			| od -A n -t u2 | xargs | tr ' ' .); \
		break; \
	done; \
	if [ -n "$$me_ver" ]; then \
		cp $(src-dir)/intel-sps.json $@; \
	else \
		cp $(src-dir)/intel-me.json $@; \
		me_ver=$$(strings -a "$$me" \
			| grep -m1 -Eo 'ME[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' \
			| sed 's/^ME//'); \
	fi; \
	if [ -z "$$me_ver" ]; then \
		tmp=$$(mktemp -d); \
		if [ "$(CONFIG_NEED_IFWI)" = "y" ]; then \
			$(IFWITOOL) "$$me" extract -f "$$tmp/NFTP" -n NFTP >/dev/null 2>&1; \
		elif [ "$(CONFIG_SOC_INTEL_CSE_HAVE_SPEC_SUPPORT)" = "y" ]; then \
			layout=$$($(CSE_SERGER) "$$me" print-layout -v $(CONFIG_CSE_BPDT_VERSION) 2>/dev/null || true); \
			last_bpdt=$$(printf '%s\n' "$$layout" | sed -n 's/^BP\([0-9]\+\) offset.*/\1/p' | sort -n | tail -1); \
			for bpdt in $$(seq "$$last_bpdt" -1 1); do \
				offset=$$(printf '%s\n' "$$layout" | awk "/BP$$bpdt offset/{print \$$NF}"); \
				size=$$(printf '%s\n' "$$layout" | awk "/BP$$bpdt size/{print \$$NF}"); \
				[ $$((size)) -ne 0 ] && break; \
			done; \
			dd if="$$me" of="$$tmp/bpdt.bin" bs=$$((size)) skip=$$((offset)) count=1 iflag=skip_bytes 2>/dev/null; \
			$(CSE_SERGER) "$$tmp/bpdt.bin" dump -o "$$tmp" -n NFTP >/dev/null 2>&1; \
		else \
			$(CSE_FPT) "$$me" dump -o "$$tmp" -n NFTP >/dev/null 2>&1;\
		fi; \
		manifest=$$(grep -aboF -m1 '$$MN2' "$$tmp/NFTP" 2>/dev/null | cut -d: -f1); \
		if [ -n "$$manifest" ]; then \
			me_ver=$$(dd if="$$tmp/NFTP" skip=$$((manifest + 8)) bs=1 count=8 2>/dev/null \
				| od -A n -t u2 | xargs | tr ' ' .); \
		fi; \
		rm -rf "$$tmp"; \
	fi; \
	if [ -n "$$me_ver" ]; then \
		sed -i "s/<software_version>/$$me_ver/" $@; \
	else \
		sed -i "/software-version/d" $@; \
	fi
	if [ -s "$(CONFIG_ME_BIN_PATH)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(CONFIG_ME_BIN_PATH))" \
			--hash "$$(sha256sum "$(CONFIG_ME_BIN_PATH)" | cut -d' ' -f1)"; \
	fi


# Derive the IFD revision from the git repository that contains IFD_BIN_PATH.
# For Dasharo builds this is 3rdparty/dasharo-blobs.  The IFD binary itself
# carries no embedded version string, so the enclosing repo commit hash is
# used as a proxy, matching the same approach used for coreboot and payloads.
# 3rdparty/dasharo-blobs keeps ifd version number in README.md.
$(build-dir)/intel-ifd.json: $(src-dir)/intel-ifd.json $(CONFIG_IFD_BIN_PATH) | $(build-dir)/goswid
	cp $< $@
	set -e; \
	ifd_git_root=$$(git -C "$$(dirname "$(CONFIG_IFD_BIN_PATH)")" rev-parse --show-toplevel 2>/dev/null); \
	ifd_readme="$$(dirname "$(CONFIG_IFD_BIN_PATH)")/README.md"; \
	ifd_version=""; \
	if [ -n "$$ifd_git_root" ]; then \
		if [ -f "$$ifd_readme" ]; then \
			ifd_version=$$(grep -A1 -E 'Intel Flash Descriptor' "$$ifd_readme" \
				| grep -i 'Version:' \
				| grep -Eo 'v([^,]+)'); \
		fi; \
		ifd_comm_hash=$$(git -C "$$ifd_git_root" log -n 1 --format="%cs_%H"); \
		sed -i -e "s/<software_version>/$$ifd_comm_hash/" \
			-e "s/<colloquial_version>/$$ifd_version/" $@; \
	else \
		sed -i -e "/software-version/d" -e "/colloquial-version/d" $@; \
	fi
	if [ -s "$(CONFIG_IFD_BIN_PATH)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(CONFIG_IFD_BIN_PATH))" \
			--hash "$$(sha256sum "$(CONFIG_IFD_BIN_PATH)" | cut -d' ' -f1)"; \
	fi

# Extract EC firmware version from the EC binary.  The version string is
# embedded as an ASCII symbol matching the pattern _VERSION=<value>, e.g.
# 76EC_VERSION=2025-10-31_af6bd04 for Dasharo/System76 EC firmware.
$(build-dir)/generic-ec.json: $(src-dir)/generic-ec.json $(CONFIG_SBOM_EC_BIN_PATH) | $(build-dir)
	cp $< $@
	set -e; \
	ec_ver=$$(strings -a "$(CONFIG_SBOM_EC_BIN_PATH)" \
		| grep -m1 '_VERSION=' \
		| sed 's/.*_VERSION=//'); \
	if [ -n "$$ec_ver" ]; then \
		sed -i "s/<software_version>/$$ec_ver/" $@; \
	else \
		sed -i "/software-version/d" $@; \
	fi

# Extract the ACM revision, date and hash.  The ACM header has a BCD date
# field at offset 0x14: byte[20]=day, byte[21]=month, bytes[22-23 LE
# 16-bit]=year.  This mirrors the Intel microcode date layout (same BCD
# encoding, different offsets).  The ACM revision is the software version and
# the date the colloquial version, as for FSP; when the ACM carries no revision,
# the date is the software version too.  The product family is the platform the
# ACM is released for (SBOM_ACM_PRODUCT_FAMILY, set by the SoC).  All template
# placeholders are replaced before goswid rewrites the file, because goswid
# escapes '<' and '>' in its JSON output.  The SHA-256 of the ACM, as Intel
# ships it (uncompressed), is added to the entry so the component carries
# integrity info.
#
# The ACM revision is the AcmRevision field (offset 37, 3 bytes:
# major.minor.build, shown as hexadecimal XX.YY.ZZ) of the Chipset AC Module
# Information Table, present from table version 6.  The table starts the User
# Area, which follows the header and the scratch area.  Per the Intel TXT MLE
# Developer's Guide (315168), "Authenticated Code Module Format":
#   header version 0.0: 128 + KeySize*4 (RSAPubKey) + 4 (RSAPubExp)
#                           + KeySize*4 (RSASig) + ScratchSize*4
#   header version 3.0: 128 + KeySize*4 (RSAPubKey) + KeySize*4 (RSASig)
#                           + ScratchSize*4 (no exponent, fixed to 0x10001)
# with HeaderVersion at offset 8, KeySize at 120 and ScratchSize at 124, both
# in dwords.  The table is accepted only if it carries the information table
# UUID; otherwise, for other header versions, or for table versions below 6,
# there is no revision.
#
# The ACM is read from the ROM image when running 'make sbom', so the SBOM
# reflects any post-build ACM swap, or when the build does not provide the ACM
# file.  Otherwise, and when extraction fails, the ACM file given in the build
# (INTEL_TXT_BIOSACM_FILE, INTEL_TXT_SINITACM_FILE) is used, which lets the
# initial build describe it without reading a ROM that does not exist yet.
# cbfstool decompresses the SINIT ACM on extraction so the offsets and the hash
# are identical for both sources and both ACM types.
#
# NOTE: $(obj)/coreboot.rom is intentionally NOT listed as a make prerequisite.
# Adding it as a prereq would cause 'make sbom' (and especially 'make -B sbom')
# to rebuild the entire ROM.  Instead, these JSON files depend on the phony
# 'sbom-acm-force' target, so they are always regenerated fresh from whatever
# ROM is already on disk.  Do not delete them from an order-only prerequisite
# of sbom.uswid: make runs order-only prerequisites after the normal ones, so
# the files would be removed after being generated and before goswid uses them.
#
# $(1): CBFS name of the ACM, $(2): ACM file given in the build (may be empty)
intel-acm-sbom = \
	if [ -n "$(CONFIG_SBOM_ACM_PRODUCT_FAMILY)" ]; then \
		sed -i "s|<acm_product_family>|$(CONFIG_SBOM_ACM_PRODUCT_FAMILY)|" $@; \
	else \
		sed -i "/<acm_product_family>/d" $@; \
	fi; \
	acm_tmp=$$(mktemp); \
	acm_src=; \
	if [ "$(filter sbom,$(MAKECMDGOALS))" = "sbom" ] || [ -z "$(2)" ]; then \
		if $(CBFSTOOL) $(obj)/coreboot.rom extract -r COREBOOT \
		    -n $(1) -f $$acm_tmp 2>/dev/null; then \
			acm_src=$$acm_tmp; \
		fi; \
	fi; \
	if [ -z "$$acm_src" ] && [ -s "$(2)" ]; then \
		acm_src="$(2)"; \
	fi; \
	if [ -n "$$acm_src" ]; then \
		year=$$(hexdump --skip 22 --length 2 --format '"%04x"' "$$acm_src"); \
		month=$$(hexdump --skip 21 --length 1 --format '"%02x"' "$$acm_src"); \
		day=$$(hexdump   --skip 20 --length 1 --format '"%02x"' "$$acm_src"); \
		hdr_ver=$$(od -An -tx4 -j8 -N4 "$$acm_src" | tr -d ' '); \
		key_size=$$(od -An -tu4 -j120 -N4 "$$acm_src" | tr -d ' '); \
		scratch_size=$$(od -An -tu4 -j124 -N4 "$$acm_src" | tr -d ' '); \
		case "$$hdr_ver" in \
		00000000) exp_size=4 ;; \
		00030000) exp_size=0 ;; \
		*) exp_size= ;; \
		esac; \
		acm_rev=; \
		if [ -n "$$exp_size" ]; then \
			info=$$((128 + key_size * 4 + exp_size + key_size * 4 + scratch_size * 4)); \
			uuid=$$(od -An -tx4 -j$$info -N16 "$$acm_src" | tr -d ' \n'); \
			info_ver=$$(od -An -tu1 -j$$((info + 17)) -N1 "$$acm_src" | tr -d ' '); \
			if [ "$$uuid" = "7fc03aaa18db46a78f69ac2e5a7f418d" ] && \
			   [ "$${info_ver:-0}" -ge 6 ]; then \
				acm_rev=$$(od -An -tx1 -j$$((info + 37)) -N3 "$$acm_src" | \
					tr 'a-f' 'A-F' | awk '{ print $$1 "." $$2 "." $$3 }'); \
			fi; \
		fi; \
		sed -i -e "s/<software_version>/$${acm_rev:-$$year-$$month-$$day}/" \
			-e "s/<colloquial_version>/$$year-$$month-$$day/" $@; \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(1)" \
			--hash "$$(sha256sum "$$acm_src" | cut -d' ' -f1)"; \
	else \
		sed -i -e "/software-version/d" -e "/<colloquial_version>/d" $@; \
	fi; \
	rm -f $$acm_tmp

$(build-dir)/intel-bios-acm.json: $(src-dir)/intel-bios-acm.json sbom-acm-force | $(build-dir) $(build-dir)/goswid
	cp $< $@
	$(call intel-acm-sbom,$(CONFIG_INTEL_TXT_CBFS_BIOS_ACM),$(sbom-bios-acm-file))

$(build-dir)/intel-sinit-acm.json: $(src-dir)/intel-sinit-acm.json sbom-acm-force | $(build-dir) $(build-dir)/goswid
	cp $< $@
	$(call intel-acm-sbom,$(CONFIG_INTEL_TXT_CBFS_SINIT_ACM),$(sbom-sinit-acm-file))

# coreboot stores the FSP components as cbfstool adds them: FSP-T and FSP-M
# placed execute-in-place are rebased to their flash address, so their bytes,
# which coreboot measures, differ from the split Fsp_T.fd/Fsp_M.fd. Each
# component is extracted from the image (cbfstool decompresses FSP-S); when it
# differs from the split file, its hash is the extracted one.
#
# In a build the image is coreboot.pre, which holds every CBFS file before the
# SBOM is added to it (add_sbom_cbfs), so the FSP tag depends on it. A
# standalone 'make sbom' must not rebuild the image: it reads the existing
# coreboot.rom (e.g. after post-build provisioning), else coreboot.pre. Only
# the COREBOOT region is read; RW copies of a vboot image are rebased to their
# own addresses.
ifeq ($(filter sbom,$(MAKECMDGOALS)),sbom)
intel-fsp-image :=
intel-fsp-image-files = $(obj)/coreboot.rom $(obj)/coreboot.pre
else
intel-fsp-image := $(obj)/coreboot.pre
intel-fsp-image-files = $(obj)/coreboot.pre
endif

# $(1): payload file name, $(2): split FSP component, $(3): its CBFS name
intel-fsp-swid-file = \
	hash=; \
	if [ -f "$(2)" ]; then hash=$$(sha256sum "$(2)" | cut -d' ' -f1); fi; \
	for img in $(intel-fsp-image-files); do \
		[ -f "$$img" ] || continue; \
		if $(CBFSTOOL) "$$img" extract -r COREBOOT -n "$(3)" -f $@.fsp >/dev/null 2>&1; then \
			ext=$$(sha256sum $@.fsp | cut -d' ' -f1); \
			if [ "$$ext" != "$$hash" ]; then \
				echo "    SBOM      $(1): $(3) in $$(basename $$img) differs from $(notdir $(2)) (rebased), using its hash"; \
				hash=$$ext; \
			fi; \
			break; \
		fi; \
	done; \
	rm -f $@.fsp; \
	$(build-dir)/goswid add-payload-file -o $@ -i $@ --name "$(1)" \
		$$([ -z "$$hash" ] || echo --hash "$$hash")

# The FSP version is embedded in the binary, FSP spec 5.1,
# FSP_INFO_HEADER, it lives in the firmware file with
# guid 912740be-2284-4734-b971-84b027353f0c
# In the header, ImageRevision (4 bytes LE at +12) decodes as
# Major.Minor.Revision.Build, one byte each. When HeaderRevision (+11) is >= 6,
# ExtendedImageRevision (2 bytes LE at +76) holds additional bytes:
#	Revision = ExtendedImageRevision[15:8] << 8 | ImageRevision[15:8]
#	Build = ExtendedImageRevision[7:0] << 8 | ImageRevision[7:0]
$(build-dir)/intel-fsp.json: $(src-dir)/intel-fsp.json $(CONFIG_FSP_FD_PATH) $(CONFIG_FSP_S_FILE) $(CONFIG_FSP_T_FILE) $(CONFIG_FSP_M_FILE) $(intel-fsp-image) | $(build-dir)/goswid
	cp $< $@
	set -e; \
	fsp_bin="$(CONFIG_FSP_FD_PATH)"; \
	fsp_version=""; fsph=""; \
	for offset in $$(grep -abo 'FSPH' "$$fsp_bin" 2>/dev/null | cut -d: -f1); do \
		ffs_guid=$$(hexdump --skip $$((offset - 28)) --length 16 --format '16/1 "%02x"' "$$fsp_bin"); \
		if [ "$$ffs_guid" = "be40279184223447b97184b027353f0c" ]; then \
			fsph=$$offset; \
			break; \
		fi; \
	done; \
	if [ -n "$$fsph" ]; then \
		header_rev=$$(hexdump --skip $$((fsph + 11)) --length 1 --format '"%u"' "$$fsp_bin"); \
		build=$$(hexdump --skip $$((fsph + 12)) --length 1 --format '"%u"' "$$fsp_bin"); \
		rev=$$(hexdump --skip $$((fsph + 13)) --length 1 --format '"%u"' "$$fsp_bin"); \
		minor=$$(hexdump --skip $$((fsph + 14)) --length 1 --format '"%u"' "$$fsp_bin"); \
		major=$$(hexdump --skip $$((fsph + 15)) --length 1 --format '"%u"' "$$fsp_bin"); \
		if [ "$$header_rev" -ge 6 ]; then \
			build_hi=$$(hexdump --skip $$((fsph + 76)) --length 1 --format '"%u"' "$$fsp_bin"); \
			rev_hi=$$(hexdump --skip $$((fsph + 77)) --length 1 --format '"%u"' "$$fsp_bin"); \
			build=$$(( (build_hi << 8) | build )); \
			rev=$$(( (rev_hi << 8) | rev )); \
		fi; \
		fsp_version=$$(printf '%X.%X.%X.%X' "$$major" "$$minor" "$$rev" "$$build"); \
	fi; \
	fsp_bios_version=""; sku_type=""; \
	if [ "$(CONFIG_FSP_USE_REPO)" = "y" ]; then \
		fsp_git_root=$$(git -C "$$(dirname "$(CONFIG_FSP_FD_PATH)")" rev-parse --show-toplevel 2>/dev/null); \
		fsp_fd_abs=$$(realpath "$(CONFIG_FSP_FD_PATH)" 2>/dev/null || printf '%s\n' "$(CONFIG_FSP_FD_PATH)"); \
		fsp_rel=$${fsp_fd_abs#$$fsp_git_root/}; \
		sku_type=$${fsp_rel%/*}; \
		sku_type=$${sku_type##*/}; \
		fsp_bios_version=$$(git -C "$$fsp_git_root" log --format=%s -- "$$fsp_rel" 2>/dev/null \
			| grep -o -m1 -E '\([0-9]+_[0-9]+\)' | tr -d '()'); \
	fi; \
	fsp_package=$$(printf '%s\n' "$(CONFIG_FSP_FD_PATH)" | sed -n 's#.*/\([A-Za-z0-9]*\)FspBinPkg/.*#\1#p'); \
	sed -i "s|<software_version>|$$fsp_version|" $@; \
	sed -i "s|<colloquial_version>|$$fsp_version Bios: $$fsp_bios_version|" $@; \
	sed -i "s|<sku_type>|$$sku_type|" $@; \
	if [ -n "$$fsp_package" ]; then sed -i "s|<fsp_package>|$$fsp_package|" $@; else sed -i "/<fsp_package>/d" $@; fi

ifneq ($(CONFIG_FSP_S_FILE),)
	echo "    SBOM      Adding FSP-S"
	$(call intel-fsp-swid-file,FSP-S,$(CONFIG_FSP_S_FILE),$(FSP_S_CBFS))
endif
ifneq ($(CONFIG_FSP_T_FILE),)
	echo "    SBOM      Adding FSP-T"
	$(call intel-fsp-swid-file,FSP-T,$(CONFIG_FSP_T_FILE),$(FSP_T_CBFS))
endif
ifneq ($(CONFIG_FSP_M_FILE),)
	echo "    SBOM      Adding FSP-M"
	$(call intel-fsp-swid-file,FSP-M,$(CONFIG_FSP_M_FILE),$(FSP_M_CBFS))
endif

# Extract AGESA version from the binary blob. AGESA binaries often embed a
# version string like "AGESA!YYYYY_X.X.X.X" or similar.  We search for the
# first string matching a common AGESA version pattern; if none is found the
# version field is omitted and the generic template description is used.
$(build-dir)/amd-agesa.json: $(src-dir)/amd-agesa.json $(CONFIG_AGESA_BINARY_PI_FILE) | $(build-dir)
	cp $< $@
	set -e; \
	agesa_ver=$$(strings -a "$(CONFIG_AGESA_BINARY_PI_FILE)" \
		| grep -m1 -Eo 'AGESA![A-Za-z0-9_]+|[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+'); \
	if [ -n "$$agesa_ver" ]; then \
		sed -i "s/<software_version>/$$agesa_ver/" $@; \
	else \
		sed -i "/software-version/d" $@; \
	fi

$(CONFIG_AMD_OPENSIL_PATH)/.git:
	git submodule update --init --checkout $(patsubst $(top)/%,%,$(CONFIG_AMD_OPENSIL_PATH))

# Record the git commit of the openSIL source tree as the SBOM version, in the
# human-readable "<commit-date>_<hash>" form used for the other git-backed deps.
# openSIL is an open-source library checked out as a submodule or external
# repo, so the commit is the canonical version identifier.
$(build-dir)/amd-opensil.json: $(src-dir)/amd-opensil.json $(CONFIG_AMD_OPENSIL_PATH)/.git
	cp $< $@
	set -e; \
	opensil_path='$(CONFIG_AMD_OPENSIL_PATH)'; \
	if [ -z "$$opensil_path" ] || [ ! -d "$$opensil_path" ]; then \
		for p in src/vendorcode/amd/opensil/*/opensil; do \
			if [ -e "$$p/.git" ]; then opensil_path="$$p"; break; fi; \
		done; \
	fi; \
	if [ -n "$$opensil_path" ] && [ -d "$$opensil_path" ]; then \
		comm_hash=$$(git -c safe.directory='*' -C "$$opensil_path" log -n 1 --format="%cs_%H" 2>/dev/null); \
		if [ -n "$$comm_hash" ]; then \
			sed -i -e "s/<software_version>/$$comm_hash/" $@; \
		else \
			sed -i "/software-version/d" $@; \
		fi; \
	else \
		sed -i "/software-version/d" $@; \
	fi


# Intel microcode files may hold several updates concatenated, e.g. one per
# platform ID of a CPUID (06-8f-08 carries two). The TotalSize field of each
# update header (offset 32, 0 meaning 2048 bytes) gives where the next one
# starts; the chain must end exactly at the end of the file.
#
# intel-ucode-count prints the number of updates in file $(1), or 0 when the
# headers do not chain over the whole file (the tag rule then reports why). It
# runs at parse time, so that every update gets its own SBOM file: goswid links
# only the first tag of each input file to the coreboot tag.
intel-ucode-count = $(shell f='$(1)'; size=$$(stat -c %s "$$f" 2>/dev/null || echo 0); off=0; n=0; \
	while [ $$((off + 48)) -le $$size ]; do \
		t=$$(od -An -tu4 -j $$((off + 32)) -N4 "$$f" | tr -d ' '); \
		[ "$$t" != 0 ] || t=2048; \
		[ $$((off + t)) -le $$size ] || break; \
		off=$$((off + t)); n=$$((n + 1)); \
	done; \
	if [ $$off -eq $$size ]; then echo $$n; else echo 0; fi)

# SBOM files of microcode file $(1): one for a file holding a single update,
# one per update (numbered from 1) otherwise.
intel-ucode-swids = $(if $(filter-out 0 1,$(call intel-ucode-count,$(1))), \
	$(foreach i,$(shell seq $(call intel-ucode-count,$(1))),$(build-dir)/intel-microcode-$(notdir $(1))-$(i).json), \
	$(build-dir)/intel-microcode-$(notdir $(1)).json)

# Recipe of the SBOM file of microcode file $(1): of its update number $(2), or
# of the whole file when $(2) is empty, which must then be a single update.
# Each tag gets a deterministic UUIDv5 generated by goswid from its name: the
# file name, plus the update's platform ID mask when the file holds several.
# The version and date come from the update's header, the hash covers the
# update's bytes only.
intel-ucode-swid = \
	cp $< $@; \
	set -e; \
	f='$(1)'; size=$$(stat -c %s "$$f"); off=0; idx=1; \
	while :; do \
		if [ $$((off + 48)) -gt $$size ]; then \
			echo "$$f: no microcode update header at offset $$off ($$size bytes)" >&2; exit 1; \
		fi; \
		total=$$(od -An -tu4 -j $$((off + 32)) -N4 "$$f" | tr -d ' '); \
		[ "$$total" != 0 ] || total=2048; \
		if [ $$((off + total)) -gt $$size ]; then \
			echo "$$f: update at offset $$off claims $$total bytes, beyond the end of the file ($$size bytes)" >&2; exit 1; \
		fi; \
		[ $$idx -lt $(or $(2),1) ] || break; \
		off=$$((off + total)); idx=$$((idx + 1)); \
	done; \
	if [ -z "$(2)" ] && [ $$total -ne $$size ]; then \
		o=0; \
		while [ $$((o + 48)) -le $$size ]; do \
			t=$$(od -An -tu4 -j $$((o + 32)) -N4 "$$f" | tr -d ' '); \
			[ "$$t" != 0 ] || t=2048; \
			if [ $$((o + t)) -gt $$size ]; then \
				echo "$$f: update at offset $$o claims $$t bytes, beyond the end of the file ($$size bytes)" >&2; exit 1; \
			fi; \
			o=$$((o + t)); \
		done; \
		echo "$$f: $$((size - o)) bytes after the microcode update(s) ending at offset $$o are no update" >&2; exit 1; \
	fi; \
	upd=$@.bin; \
	tail -c +$$((off + 1)) "$$f" | head -c $$total > "$$upd"; \
	rev=$$(hexdump --skip 4 --length 4 --format '"0x%x"' "$$upd"); \
	year=$$(hexdump --skip 8 --length 2 --format '"%04x"' "$$upd"); \
	day=$$(hexdump --skip 10 --length 1 --format '"%02x"' "$$upd"); \
	month=$$(hexdump --skip 11 --length 1 --format '"%02x"' "$$upd"); \
	cpuid=$$(hexdump --skip 12 --length 4 --format '"0x%x"' "$$upd"); \
	pf=$$(hexdump --skip 24 --length 4 --format '"%x"' "$$upd"); \
	name=$(notdir $(1))$(if $(2),-pf$$pf); \
	tag_id=$$($(build-dir)/goswid generate-tag-id -n "Intel-Microcode-$$name"); \
	sed -i \
		-e "s/<tag_id>/$$tag_id/" \
		-e "s/<ucode_name>/$$name/" \
		-e "s/<software_version>/$$rev/" \
		-e "s/<colloquial_version>/$$year-$$month-$$day/" \
		-e "s/<cpuid>/$$cpuid/" \
		$@; \
	$(build-dir)/goswid add-payload-file -o $@ -i $@ \
		--name "$(notdir $(1))$(if $(2), (update $(2), platform IDs 0x$$pf))" \
		--hash "$$(sha256sum "$$upd" | cut -d' ' -f1)"; \
	rm -f "$$upd"

# A microcode file holding a single update
$(build-dir)/intel-microcode-%.json: $(src-dir)/intel-microcode.json 3rdparty/intel-microcode/intel-ucode/% | $(build-dir) $(build-dir)/goswid
	$(call intel-ucode-swid,$(word 2,$^),)

# One update of a microcode file holding several
define intel-ucode-update-rule
$(build-dir)/intel-microcode-$(notdir $(1))-$(2).json: $(src-dir)/intel-microcode.json $(1) | $(build-dir) $(build-dir)/goswid
	$$(call intel-ucode-swid,$(1),$(2))
endef
# cpu_microcode_bins is populated by SoC Makefile.mks included after src/sbom,
# so the rules are generated once all of them are processed (as for AMD below).
postinclude-hooks += $$(foreach ucode,$$(filter 3rdparty/intel-microcode/intel-ucode/%,$$(cpu_microcode_bins)), \
	$$(if $$(filter-out 0 1,$$(call intel-ucode-count,$$(ucode))), \
		$$(foreach i,$$(shell seq $$(call intel-ucode-count,$$(ucode))),$$(eval $$(call intel-ucode-update-rule,$$(ucode),$$(i))))))

# Generate per-file SBOM rules for AMD microcode patches.
# AMD microcode filenames vary widely across SoCs:
#   UcodePatch_CZN_A0.bin       (cezanne, picasso)
#   Typex66_0_0_0_UCodePatch.bin (genoa)
#   TypeId0x66_UcodePatch_A0.bin (glinda, phoenix, mendocino)
# A static pattern rule cannot handle these because make's % cannot match a
# variable prefix.  Instead we generate one explicit rule per file via eval.
# The AMD ucode patch binary header layout (all SoCs, per AMD PSP spec):
#   offset 0x00, 2 bytes LE: year  (BCD)
#   offset 0x02, 1 byte:     day   (BCD)
#   offset 0x03, 1 byte:     month (BCD)
#   offset 0x04, 4 bytes:    patch ID (HEX)
define amd-ucode-sbom-rule
$(build-dir)/amd-microcode-$(basename $(notdir $(1))).json: $(src-dir)/amd-microcode.json $(1) | $(build-dir) $(build-dir)/goswid
	cp $$< $$@
	year=$$$$(hexdump --skip 0 --length 2 --format '"%04x"' $(1)); \
	day=$$$$(hexdump --skip 2 --length 1 --format '"%02x"' $(1)); \
	month=$$$$(hexdump --skip 3 --length 1 --format '"%02x"' $(1)); \
	patch=$$$$(hexdump --skip 4 --length 4 --format '"0x%08x"' $(1)); \
	sed -i "s/<software_version>/$$$$patch $$$$year-$$$$month-$$$$day/" $$@
	$(build-dir)/goswid add-payload-file -o $$@ -i $$@ \
		--name "$(notdir $(1))" \
		--hash "$$$$(sha256sum "$(1)" | cut -d' ' -f1)"
	$(if $(CONFIG_SBOM_AMD_BLOB_LICENSE),$(build-dir)/goswid add-license -o $$@ -i $$@ "$(CONFIG_SBOM_AMD_BLOB_LICENSE)",:)
endef
# amd_microcode_bins is populated in src/soc/amd/common/block/cpu/Makefile.mk,
# which is included several rounds after src/sbom (breadth-first traversal).
# Deferring via postinclude-hooks ensures the rules are generated after all
# Makefile.mks have been processed and amd_microcode_bins is fully populated.
postinclude-hooks += $$(foreach ucode,$$(amd_microcode_bins),$$(eval $$(call amd-ucode-sbom-rule,$$(ucode))))

vboot-gitdir := 3rdparty/vboot/.git

$(build-dir)/vboot.json: $(src-dir)/vboot.json $(vboot-gitdir) | $(build-dir) $(build-dir)/goswid
	cp $< $@
	git_comm_hash=$$(git --git-dir 3rdparty/vboot/.git log -n 1 --format="%cs_%H"); \
	git_latest_rel=$$(git --git-dir 3rdparty/vboot/.git tag --merged HEAD --sort=-creatordate | head -n1); \
	sed -i -e "s/<colloquial_version>/$$git_latest_rel/" -e "s/<software_version>/$$git_comm_hash/" $@
	# vboot is built from source as a per-stage static library rather than a
	# single blob; hash the ramstage vboot_fw.a as the representative artifact
	# so the component carries integrity info (CRA Annex I hash carry-through).
	if [ -s "$(VBOOT_LIB_ramstage)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(VBOOT_LIB_ramstage))" \
			--hash "$$(sha256sum "$(VBOOT_LIB_ramstage)" | cut -d' ' -f1)"; \
	fi

ifeq ($(CONFIG_BUILD_IPXE),y)
ipxe-gitdir := payloads/external/iPXE/ipxe/.git
ifeq ($(CONFIG_EDK2_ENABLE_IPXE),y)
$(ipxe-gitdir): $(CONFIG_PAYLOAD_FILE)
else
$(ipxe-gitdir): ipxe
endif
endif

# iPXE
$(build-dir)/payload-iPXE.json: $(src-dir)/payload-iPXE.json $(ipxe-gitdir) | $(build-dir) $(build-dir)/goswid $(ipxe-swid-ready-dep)
	cp $< $@
	git_comm_hash=$$(git --git-dir payloads/external/iPXE/ipxe/.git log -n 1 --format="%cs_%H"); \
	git_latest_rel=$$(git --git-dir payloads/external/iPXE/ipxe/.git tag --merged HEAD --sort=-creatordate | head -n1); \
	sed -i -e "s/<colloquial_version>/$$git_latest_rel/" -e "s/<software_version>/$$git_comm_hash/" $@
	# Hash the built iPXE ROM image (the artifact linked into CBFS) so the
	# component carries integrity info (CRA Annex I hash carry-through).
	if [ -s "payloads/external/iPXE/ipxe/ipxe.rom" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "ipxe.rom" \
			--hash "$$(sha256sum "payloads/external/iPXE/ipxe/ipxe.rom" | cut -d' ' -f1)"; \
	fi

# STM: built from the 3rdparty/stm submodule unless STM_BINARY_FILE points
# to a prebuilt binary. Record the submodule revision only when the binary is
# the one built from it, and hash the uncompressed binary (CBFS stores it
# LZMA-compressed) so the component carries integrity info.
stm-src-bin := 3rdparty/stm/Stm/build/StmPkg/Core/stm.bin

$(build-dir)/stm.json: $(src-dir)/stm.json | $(build-dir) $(build-dir)/goswid $(stm-swid-ready-dep)
	cp $< $@
	set -e; \
	if [ "$(sbom-stm-file)" = "$(stm-src-bin)" ] && \
	   git -C 3rdparty/stm rev-parse --git-dir >/dev/null 2>&1; then \
		git_comm_hash=$$(git -C 3rdparty/stm log -n 1 --format="%cs_%H"); \
		git_latest_rel=$$(git -C 3rdparty/stm tag --merged HEAD --sort=-creatordate | head -n1); \
		sed -i -e "s/<colloquial_version>/$$git_latest_rel/" -e "s/<software_version>/$$git_comm_hash/" $@; \
	else \
		sed -i -e "/<colloquial_version>/d" -e "/<software_version>/d" $@; \
	fi; \
	if [ -s "$(sbom-stm-file)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "stm.bin" \
			--hash "$$(sha256sum "$(sbom-stm-file)" | cut -d' ' -f1)"; \
	fi

# edk2-platforms is a separate git repository compiled into the edk2 payload.
# Record its "<commit-date>_<hash>" (software-version) and latest release tag
# (colloquial-version), mirroring the iPXE/payload git-backed component rules.
# The checkout lives at a deterministic path created during the edk2 payload
# build.
#
# edk2-platforms has no standalone binary artifact (it is linked into the edk2
# payload FD), so for a component-integrity hash we record a sha256 over its
# exact source tree via `git archive HEAD`.  The archive is deterministic for a
# given commit (git sets entry mtimes to the commit time and prepends the commit
# id), so this is a stable, reproducible digest of what was actually built.  It
# lets the component carry a CRA payload.file[].hash like the binary blobs do.
ifeq ($(CONFIG_SBOM_EDK2_PLATFORMS_GENERATE),y)
edk2-platforms-git-dir := payloads/external/edk2/workspace/edk2-platforms
edk2-platforms-gitdir := $(shell git -C $(edk2-platforms-git-dir) rev-parse --absolute-git-dir 2>/dev/null)
endif

$(build-dir)/payload-edk2-platforms.json: $(src-dir)/payload-edk2-platforms.json $(edk2-platforms-gitdir) | $(build-dir) $(build-dir)/goswid $(edk2-platforms-swid-ready-dep)
	cp $< $@
	set -e; \
	if [ -e "$(edk2-platforms-git-dir)/.git" ]; then \
		git_comm_hash=$$(git --git-dir $(edk2-platforms-git-dir)/.git log -n 1 --format="%cs_%H"); \
		git_latest_rel=$$(git --git-dir $(edk2-platforms-git-dir)/.git tag --merged HEAD --sort=-creatordate | head -n1); \
		sed -i -e "s/<colloquial_version>/$$git_latest_rel/" -e "s/<software_version>/$$git_comm_hash/" $@; \
		src_hash=$$(git --git-dir $(edk2-platforms-git-dir)/.git archive HEAD | sha256sum | cut -d' ' -f1); \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "edk2-platforms-source.tar" \
			--hash "$$src_hash"; \
	else \
		sed -i -e "/<colloquial_version>/d" -e "/<software_version>/d" $@; \
	fi

# VGA BIOS OptionROMs and edk2 GOP/LAN drivers are proprietary binary blobs with
# no extractable semantic version.  Record a sha256 hash of each blob (and, for
# VGA BIOS, the target PCI vendor:device ID) so the component carries integrity
# and identity info.  Vendors should prefer supplying a full SBOM via the
# matching CONFIG_SBOM_*_PATH option.  The blob is a wildcard prerequisite so the
# entry regenerates when the blob changes but a missing file never breaks make.
$(build-dir)/vga-bios.json: $(src-dir)/vga-bios.json $(wildcard $(CONFIG_VGA_BIOS_FILE)) | $(build-dir) $(build-dir)/goswid
	cp $< $@
	if [ -n "$(CONFIG_VGA_BIOS_ID)" ]; then \
		sed -i "s|<software_version>|$(CONFIG_VGA_BIOS_ID)|" $@; \
	else \
		sed -i "/<software_version>/d" $@; \
	fi
	if [ -s "$(CONFIG_VGA_BIOS_FILE)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "pci$(CONFIG_VGA_BIOS_ID).rom" \
			--hash "$$(sha256sum "$(CONFIG_VGA_BIOS_FILE)" | cut -d' ' -f1)"; \
	fi

$(build-dir)/vga-bios-second.json: $(src-dir)/vga-bios-second.json $(wildcard $(CONFIG_VGA_BIOS_SECOND_FILE)) | $(build-dir) $(build-dir)/goswid
	cp $< $@
	if [ -n "$(CONFIG_VGA_BIOS_SECOND_ID)" ]; then \
		sed -i "s|<software_version>|$(CONFIG_VGA_BIOS_SECOND_ID)|" $@; \
	else \
		sed -i "/<software_version>/d" $@; \
	fi
	if [ -s "$(CONFIG_VGA_BIOS_SECOND_FILE)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "pci$(CONFIG_VGA_BIOS_SECOND_ID).rom" \
			--hash "$$(sha256sum "$(CONFIG_VGA_BIOS_SECOND_FILE)" | cut -d' ' -f1)"; \
	fi

$(build-dir)/vga-bios-dgpu.json: $(src-dir)/vga-bios-dgpu.json $(wildcard $(CONFIG_VGA_BIOS_DGPU_FILE)) | $(build-dir) $(build-dir)/goswid
	cp $< $@
	if [ -n "$(CONFIG_VGA_BIOS_DGPU_ID)" ]; then \
		sed -i "s|<software_version>|$(CONFIG_VGA_BIOS_DGPU_ID)|" $@; \
	else \
		sed -i "/<software_version>/d" $@; \
	fi
	if [ -s "$(CONFIG_VGA_BIOS_DGPU_FILE)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "pci$(CONFIG_VGA_BIOS_DGPU_ID).rom" \
			--hash "$$(sha256sum "$(CONFIG_VGA_BIOS_DGPU_FILE)" | cut -d' ' -f1)"; \
	fi

$(build-dir)/edk2-gop.json: $(src-dir)/edk2-gop.json $(wildcard $(CONFIG_EDK2_GOP_FILE)) | $(build-dir) $(build-dir)/goswid
	cp $< $@
	# Label the component by the actual GOP driver vendor (Intel vs AMD), since
	# the same template and rule serve both EDK2_GOP_DRIVER and
	# EDK2_AMD_GOP_DRIVER builds.
	if [ "$(CONFIG_EDK2_AMD_GOP_DRIVER)" = "y" ]; then gop_vendor="AMD"; else gop_vendor="Intel"; fi; \
	sed -i -e "s/<gop_vendor>/$$gop_vendor/g" $@
	# The GOP driver blob carries no embedded version; use the enclosing git
	# repo's HEAD as a proxy: "<commit-date>_<hash>" (software-version) and the
	# latest release tag (colloquial-version), mirroring the Intel Flash
	# Descriptor rule.
	set -e; \
	gop_git_root=$$(git -C "$$(dirname "$(CONFIG_EDK2_GOP_FILE)")" rev-parse --show-toplevel 2>/dev/null); \
	if [ -n "$$gop_git_root" ]; then \
		gop_comm_hash=$$(git -C "$$gop_git_root" log -n 1 --format="%cs_%H"); \
		gop_latest_rel=$$(git -C "$$gop_git_root" tag --merged HEAD --sort=-creatordate | head -n1); \
		sed -i -e "s/<software_version>/$$gop_comm_hash/" \
		       -e "s/<colloquial_version>/$$gop_latest_rel/" $@; \
	else \
		sed -i -e "/<software_version>/d" -e "/<colloquial_version>/d" $@; \
	fi
	if [ -s "$(CONFIG_EDK2_GOP_FILE)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(CONFIG_EDK2_GOP_FILE))" \
			--hash "$$(sha256sum "$(CONFIG_EDK2_GOP_FILE)" | cut -d' ' -f1)"; \
	fi

$(build-dir)/edk2-lan-rom.json: $(src-dir)/edk2-lan-rom.json $(wildcard $(CONFIG_EDK2_LAN_ROM_DRIVER)) | $(build-dir) $(build-dir)/goswid
	cp $< $@
	sed -i "/<software_version>/d" $@
	if [ -s "$(CONFIG_EDK2_LAN_ROM_DRIVER)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(CONFIG_EDK2_LAN_ROM_DRIVER))" \
			--hash "$$(sha256sum "$(CONFIG_EDK2_LAN_ROM_DRIVER)" | cut -d' ' -f1)"; \
	fi

ifeq ($(CONFIG_PAYLOAD_LINUXBOOT),y)
# LinuxBoot is not a single git checkout; it is a Linux kernel plus a u-root
# initramfs.  There is no payloads/external/LinuxBoot/linuxboot repo to read a
# commit hash from, so the generic git-based rule below would leave both version
# fields empty.  Instead, use the configured Linux kernel version as the software
# version and record the u-root revision (resolved to the checked-out commit hash
# when the u-root clone is available) as the colloquial version.
$(payload-swid): $(payload-swid-template) | $(build-dir) $(payload-git-dir-y)/.git
	cp $< $@
	set -e; \
	kernel_ver='$(call strip_quotes,$(CONFIG_LINUXBOOT_KERNEL_VERSION))'; \
	uroot_ver='$(call strip_quotes,$(CONFIG_LINUXBOOT_UROOT_VERSION))'; \
	uroot_git=$$(find payloads/external/LinuxBoot/build/go/src -type d \
		-path '*/u-root/.git' 2>/dev/null | head -n 1); \
	if [ -n "$$uroot_git" ]; then \
		uroot_ver=$$(git --git-dir "$$uroot_git" log -n 1 --format=%H 2>/dev/null || echo "$$uroot_ver"); \
	fi; \
	if [ -n "$$kernel_ver" ]; then \
		sed -i "s/<software_version>/$$kernel_ver/" $@; \
	else \
		sed -i "/software-version/d" $@; \
	fi; \
	if [ -n "$$uroot_ver" ]; then \
		sed -i "s|<colloquial_version>|u-root $$uroot_ver|" $@; \
	else \
		sed -i "/colloquial-version/d" $@; \
	fi
else
# Build payload SBOM metadata only after the payload is ready in regular builds.
# For standalone `make sbom`, use an existing checkout only.
$(payload-swid): $(payload-swid-template) | $(build-dir) $(build-dir)/goswid $(payload-swid-ready-dep)
	cp $< $@;\
	git_comm_hash=$$(git --git-dir $(payload-git-dir-y)/.git log -n 1 --format="%cs_%H");\
	git_latest_rel=$$(git --git-dir $(payload-git-dir-y)/.git tag --merged HEAD --sort=-creatordate | head -n1); \
	sed -i -e "s/<colloquial_version>/$$git_latest_rel/" -e "s/<software_version>/$$git_comm_hash/" $@;
	if [ -s "$(CONFIG_PAYLOAD_FILE)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(CONFIG_PAYLOAD_FILE))" \
			--hash "$$(sha256sum "$(CONFIG_PAYLOAD_FILE)" | cut -d' ' -f1)"; \
	fi
endif

## Standalone SBOM regeneration target
## Rebuilds build/sbom/sbom.uswid from existing build artifacts without
## requiring a full rebuild. Useful for updating the SBOM after post-build
## blob patching (e.g. btg_provision, create_eom swap ACMs).
## Usage: make sbom
## To re-inject into the ROM after regenerating: use cbfstool manually or
## via build.sh once that integration is added.
##
## ACM JSON files are always regenerated so they are freshly extracted from
## the current ROM, reflecting any post-build ACM swap.
.PHONY: sbom sbom-acm-force

sbom-acm-force:

## FIXME: this target doesn't reliably trigger regeneration of sbom.uswid, which
##        is easy to see by running `make --debug=b sbom` once or twice
sbom: $(build-dir)/sbom.uswid
