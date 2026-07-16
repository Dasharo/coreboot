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
ifeq ($(CONFIG_PAYLOAD_EDK2), y)
payload-git-dir-y = payloads/external/edk2/workspace/$(word 3,$(subst /, ,$(CONFIG_EDK2_REPOSITORY)))
endif
ifneq ($(payload-git-dir-y),)
# only proceed with payload sbom data, if one of the above payloads were selected (should be guarded by Kconfig as well)
# e.g. payload-git-dir-y=payloads/external/SeaBIOS/seabios -> payload-json-file=$(build-dir)/payload-SeaBIOS.json
ifeq ($(CONFIG_PAYLOAD_EDK2), y)
payload-swid          = $(build-dir)/payload-edk2.json
payload-swid-template = $(src-dir)/payload-edk2.json
else
payload-swid = $(build-dir)/payload-$(subst /,,$(dir $(patsubst payloads/external/%,%,$(payload-git-dir-y)))).json
payload-swid-template = $(patsubst $(build-dir)/%.json,$(src-dir)/%.json,$(payload-swid))
endif
endif
endif

# edk2 payload: also set swid variables when SBOM_PAYLOAD_GENERATE is not yet
# enabled in .config (e.g. configs predating edk2 support in SBOM_PAYLOAD_GENERATE).
ifeq ($(CONFIG_SBOM_PAYLOAD)$(CONFIG_PAYLOAD_EDK2), yy)
payload-git-dir-y     := payloads/external/edk2/workspace/$(word 3,$(subst /, ,$(CONFIG_EDK2_REPOSITORY)))
payload-swid          := $(build-dir)/payload-edk2.json
payload-swid-template := $(src-dir)/payload-edk2.json
endif

# Keep standalone "make sbom" rebuilds read-only with respect to payloads:
# use already checked-out repositories for version extraction and avoid
# re-triggering payload fetch/build targets (especially with `make -B sbom`).
ifeq ($(filter sbom,$(MAKECMDGOALS)),sbom)
payload-swid-ready-dep := $(wildcard $(payload-git-dir-y)/.git)
ipxe-swid-ready-dep := $(wildcard payloads/external/iPXE/ipxe/.git)
edk2-platforms-swid-ready-dep := $(wildcard payloads/external/edk2/workspace/edk2-platforms/.git)
else
payload-swid-ready-dep := $(CONFIG_PAYLOAD_FILE)
ipxe-swid-ready-dep := payloads/external/iPXE/ipxe/ipxe.rom
# edk2-platforms is checked out as part of the edk2 payload build, so gate its
# SBOM extraction on the built payload file being ready.
edk2-platforms-swid-ready-dep := $(CONFIG_PAYLOAD_FILE)
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
# TODO think about just using one CoSWID tag for all intel-microcode instead of one for each. maybe put each microcode into files entity of CoSWID tag?
swid-files-$(CONFIG_SBOM_INTEL_MICROCODE) += $(patsubst 3rdparty/intel-microcode/intel-ucode/%, $(build-dir)/intel-microcode-%.json, $(filter 3rdparty/intel-microcode/intel-ucode/%, $(cpu_microcode_bins)))
swid-files-$(CONFIG_SBOM_AMD_MICROCODE) += $(foreach ucode,$(amd_microcode_bins),$(build-dir)/amd-microcode-$(basename $(notdir $(ucode))).json)
swid-files-$(CONFIG_SBOM_INTEL_FSP) += $(if $(CONFIG_SBOM_INTEL_FSP_GENERATE), $(build-dir)/intel-fsp.json, $(CONFIG_SBOM_INTEL_FSP_PATH))
swid-files-$(CONFIG_SBOM_AGESA) += $(if $(CONFIG_SBOM_AGESA_GENERATE), $(build-dir)/amd-agesa.json, $(CONFIG_SBOM_AGESA_PATH))
swid-files-$(CONFIG_SBOM_OPENSIL) += $(if $(CONFIG_SBOM_OPENSIL_GENERATE), $(build-dir)/amd-opensil.json, $(CONFIG_SBOM_OPENSIL_PATH))
swid-files-$(CONFIG_SBOM_IFD) += $(if $(CONFIG_SBOM_IFD_GENERATE),$(build-dir)/intel-ifd.json,$(CONFIG_SBOM_IFD_PATH))
swid-files-$(CONFIG_SBOM_EC) += $(if $(CONFIG_SBOM_EC_GENERATE),$(build-dir)/generic-ec.json,$(CONFIG_SBOM_EC_PATH))
# For ACM entries: extract version from the ROM after provisioning (make sbom).
# The wildcard guard avoids a circular dependency during the initial 'make all'
# (coreboot.rom → sbom.uswid → acm.json → coreboot.rom): when the ROM does not
# yet exist, swid-files is left empty for ACMs so the initial SBOM builds
# without ACM version info.  After post-build ACM provisioning, run 'make sbom'
# to regenerate sbom.uswid with the real ACM versions extracted from the ROM.
swid-files-$(CONFIG_SBOM_BIOS_ACM) += \
	$(if $(CONFIG_SBOM_BIOS_ACM_GENERATE), \
		$(if $(wildcard $(obj)/coreboot.rom),$(build-dir)/intel-bios-acm.json,), \
		$(CONFIG_SBOM_BIOS_ACM_PATH))
swid-files-$(CONFIG_SBOM_SINIT_ACM) += \
	$(if $(CONFIG_SBOM_SINIT_ACM_GENERATE), \
		$(if $(wildcard $(obj)/coreboot.rom),$(build-dir)/intel-sinit-acm.json,), \
		$(CONFIG_SBOM_SINIT_ACM_PATH))

swid-files-$(CONFIG_SBOM_IPXE) += $(if $(CONFIG_SBOM_IPXE_GENERATE),$(build-dir)/payload-iPXE.json,$(CONFIG_SBOM_IPXE_PATH))

# Proprietary graphics/network binary blobs (VGA BIOS OptionROMs, edk2 GOP and
# LAN drivers).  In GENERATE mode a minimal template + blob hash is emitted;
# vendors should prefer supplying a full SBOM via the matching _PATH option.
swid-files-$(CONFIG_SBOM_VGA_BIOS) += $(if $(CONFIG_SBOM_VGA_BIOS_GENERATE),$(build-dir)/vga-bios.json,$(CONFIG_SBOM_VGA_BIOS_PATH))
swid-files-$(CONFIG_SBOM_VGA_BIOS_SECOND) += $(if $(CONFIG_SBOM_VGA_BIOS_SECOND_GENERATE),$(build-dir)/vga-bios-second.json,$(CONFIG_SBOM_VGA_BIOS_SECOND_PATH))
swid-files-$(CONFIG_SBOM_VGA_BIOS_DGPU) += $(if $(CONFIG_SBOM_VGA_BIOS_DGPU_GENERATE),$(build-dir)/vga-bios-dgpu.json,$(CONFIG_SBOM_VGA_BIOS_DGPU_PATH))
swid-files-$(CONFIG_SBOM_EDK2_GOP) += $(if $(CONFIG_SBOM_EDK2_GOP_GENERATE),$(build-dir)/edk2-gop.json,$(CONFIG_SBOM_EDK2_GOP_PATH))
swid-files-$(CONFIG_SBOM_EDK2_LAN_ROM) += $(if $(CONFIG_SBOM_EDK2_LAN_ROM_GENERATE),$(build-dir)/edk2-lan-rom.json,$(CONFIG_SBOM_EDK2_LAN_ROM_PATH))

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
cbfs-files-y += sbom
sbom-file = $(build-dir)/sbom.uswid
sbom-type = raw
endif

## Build final SBOM (Software Bill of Materials) file in uswid format

$(build-dir)/sbom.uswid: $(build-dir)/coreboot.json $$(swid-files-y) $(swid-files-compiler) | $(build-dir)/goswid $(build-dir) sbom-acm-clean
	echo "    SBOM      " $^
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
$(build-dir)/coreboot.json: $(src-dir)/coreboot.json $(coreboot-gitdir)/HEAD | $(build-dir)/goswid
	cp $< $@
	git_tree_hash=$$(git log -n 1 --format=%T);\
	git_comm_hash=$$(git log -n 1 --format=%H);\
	sed -i -e "s/<colloquial_version>/$$git_tree_hash/" -e "s/<software_version>/$$git_comm_hash/" $@;\
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

# Extract ME/TXE version from the firmware binary. Some versions
# store it as an ASCII string like: "ME16.1.40.2765".
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

$(build-dir)/intel-me.json: $(src-dir)/intel-me.json $(sbom-me-bin) | $(build-dir) $(build-dir)/goswid $(IFWITOOL) $(CSE_SERGER) $(CSE_FPT)
	cp $< $@
	me='$(sbom-me-bin)'; \
	me_ver=$$(strings -a "$$me" \
		| grep -m1 -Eo 'ME[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' \
		| sed 's/^ME//'); \
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
$(build-dir)/intel-ifd.json: $(src-dir)/intel-ifd.json $(CONFIG_IFD_BIN_PATH) | $(build-dir)/goswid
	cp $< $@
	set -e; \
	ifd_git_root=$$(git -C "$$(dirname "$(CONFIG_IFD_BIN_PATH)")" rev-parse --show-toplevel 2>/dev/null); \
	if [ -n "$$ifd_git_root" ]; then \
		ifd_comm_hash=$$(git -C "$$ifd_git_root" log -n 1 --format=%H); \
		ifd_tree_hash=$$(git -C "$$ifd_git_root" log -n 1 --format=%T); \
		sed -i -e "s/<software_version>/$$ifd_comm_hash/" \
		       -e "s/<colloquial_version>/$$ifd_tree_hash/" $@; \
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

# Extract ACM version from the ROM image.  The ACM header has a BCD date field
# at offset 0x14: byte[20]=day, byte[21]=month, bytes[22-23 LE 16-bit]=year.
# This mirrors the Intel microcode date layout (same BCD encoding, different
# offsets).  cbfstool decompresses the SINIT ACM on extraction so the offsets
# are identical for both ACM types.
# NOTE: $(obj)/coreboot.rom is intentionally NOT listed as a make prerequisite.
# Adding it as a prereq would cause 'make sbom' (and especially 'make -B sbom')
# to rebuild the entire ROM.  Instead, the 'sbom' target depends on the phony
# 'sbom-acm-clean' target which deletes these JSON files unconditionally, so
# they are always regenerated fresh from whatever ROM is already on disk.
$(build-dir)/intel-bios-acm.json: $(src-dir)/intel-bios-acm.json | $(build-dir)
	cp $< $@
	acm_tmp=$$(mktemp); \
	if $(CBFSTOOL) $(obj)/coreboot.rom extract -r COREBOOT \
	    -n $(CONFIG_INTEL_TXT_CBFS_BIOS_ACM) -f $$acm_tmp 2>/dev/null; then \
		year=$$(hexdump --skip 22 --length 2 --format '"%04x"' $$acm_tmp); \
		month=$$(hexdump --skip 21 --length 1 --format '"%02x"' $$acm_tmp); \
		day=$$(hexdump   --skip 20 --length 1 --format '"%02x"' $$acm_tmp); \
		sed -i "s/<software_version>/$$year-$$month-$$day/" $@; \
	else \
		sed -i "/software-version/d" $@; \
	fi; \
	rm -f $$acm_tmp

$(build-dir)/intel-sinit-acm.json: $(src-dir)/intel-sinit-acm.json | $(build-dir)
	cp $< $@
	acm_tmp=$$(mktemp); \
	if $(CBFSTOOL) $(obj)/coreboot.rom extract -r COREBOOT \
	    -n $(CONFIG_INTEL_TXT_CBFS_SINIT_ACM) -f $$acm_tmp 2>/dev/null; then \
		year=$$(hexdump --skip 22 --length 2 --format '"%04x"' $$acm_tmp); \
		month=$$(hexdump --skip 21 --length 1 --format '"%02x"' $$acm_tmp); \
		day=$$(hexdump   --skip 20 --length 1 --format '"%02x"' $$acm_tmp); \
		sed -i "s/<software_version>/$$year-$$month-$$day/" $@; \
	else \
		sed -i "/software-version/d" $@; \
	fi; \
	rm -f $$acm_tmp

$(build-dir)/intel-fsp.json: $(src-dir)/intel-fsp.json $(CONFIG_FSP_S_FILE) $(CONFIG_FSP_T_FILE) $(CONFIG_FSP_M_FILE) | $(build-dir)/goswid
	cp $< $@
ifeq ($(CONFIG_FSP_USE_REPO),y)
	# Extract three facts from the FD's git history + path:
	#   - FSP release version (e.g. A.0.7E.70 or 2.11.E026.1930) -> software-version
	#   - BIOS build number   (e.g. 6114_54, from a "(NNNN_NN)" or "vNNNN_NN" token) -> colloquial-version
	#   - target + SoC package from the FD path (.../<SoC>FspBinPkg/<target>/...) -> edition / product-family
	# The commit subject holds both version tokens, e.g.
	#   "Edge Platforms PTL-H PV2 (4063_65) FSP ver 2.11.E026.1930".
	set -e; \
	fsp_git_root=$$(git -C "$$(dirname "$(CONFIG_FSP_FD_PATH)")" rev-parse --show-toplevel 2>/dev/null); \
	fsp_fd_abs=$$(realpath "$(CONFIG_FSP_FD_PATH)" 2>/dev/null || printf '%s\n' "$(CONFIG_FSP_FD_PATH)"); \
	fsp_rel=$${fsp_fd_abs#$$fsp_git_root/}; \
	fsp_subject=$$(git -C "$$fsp_git_root" log -n1 --format=%s -- "$$fsp_rel" 2>/dev/null); \
	fsp_release=$$(printf '%s\n' "$$fsp_subject" | grep -o -m1 -E '[A-Z0-9]\.[0-9]+\.[A-Fa-f0-9]+\.[0-9]+' || true); \
	fsp_bios=$$(printf '%s\n' "$$fsp_subject" | grep -o -m1 -E '\(?v?[0-9]+_[0-9]+\)?' | tr -d '()v' || true); \
	fsp_target=$$(printf '%s\n' "$(CONFIG_FSP_FD_PATH)" | sed -n 's#.*FspBinPkg/\([^/]*\)/.*#\1#p'); \
	fsp_package=$$(printf '%s\n' "$(CONFIG_FSP_FD_PATH)" | sed -n 's#.*/\([A-Za-z0-9]*\)FspBinPkg/.*#\1#p'); \
	fsp_ver="$$fsp_release"; [ -n "$$fsp_ver" ] || fsp_ver="$$fsp_bios"; \
	if [ -n "$$fsp_ver" ]; then sed -i "s|<software_version>|$$fsp_ver|" $@; else sed -i "/<software_version>/d" $@; fi; \
	if [ -n "$$fsp_bios" ]; then sed -i "s|<colloquial_version>|$$fsp_bios|" $@; else sed -i "/<colloquial_version>/d" $@; fi; \
	if [ -n "$$fsp_target" ]; then sed -i "s|<fsp_target>|$$fsp_target|" $@; else sed -i "/<fsp_target>/d" $@; fi; \
	if [ -n "$$fsp_package" ]; then sed -i "s|<fsp_package>|$$fsp_package|" $@; else sed -i "/<fsp_package>/d" $@; fi
else
	sed -i -e "/<software_version>/d" -e "/<colloquial_version>/d" -e "/<fsp_target>/d" -e "/<fsp_package>/d" $@
endif
ifneq ($(CONFIG_FSP_S_FILE),)
	echo "    SBOM      Adding FSP-S"
	$(build-dir)/goswid add-payload-file -o $@ -i $@ --name "FSP-S" \
		$(if $(wildcard $(CONFIG_FSP_S_FILE)),--hash "$$(sha256sum "$(CONFIG_FSP_S_FILE)" | cut -d' ' -f1)")
endif
ifneq ($(CONFIG_FSP_T_FILE),)
	echo "    SBOM      Adding FSP-T"
	$(build-dir)/goswid add-payload-file -o $@ -i $@ --name "FSP-T" \
		$(if $(wildcard $(CONFIG_FSP_T_FILE)),--hash "$$(sha256sum "$(CONFIG_FSP_T_FILE)" | cut -d' ' -f1)")
endif
ifneq ($(CONFIG_FSP_M_FILE),)
	echo "    SBOM      Adding FSP-M"
	$(build-dir)/goswid add-payload-file -o $@ -i $@ --name "FSP-M" \
		$(if $(wildcard $(CONFIG_FSP_M_FILE)),--hash "$$(sha256sum "$(CONFIG_FSP_M_FILE)" | cut -d' ' -f1)")
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

# Record the git commit of the openSIL source tree as the SBOM version.
# openSIL is an open-source library checked out as a submodule or external
# repo, so the commit hash is the canonical version identifier.
$(build-dir)/amd-opensil.json: $(src-dir)/amd-opensil.json | $(build-dir)
	cp $< $@
	set -e; \
	opensil_path='$(CONFIG_AMD_OPENSIL_PATH)'; \
	if [ -z "$$opensil_path" ] || [ ! -d "$$opensil_path" ]; then \
		for p in src/vendorcode/amd/opensil/*/opensil; do \
			if [ -e "$$p/.git" ]; then opensil_path="$$p"; break; fi; \
		done; \
	fi; \
	if [ -n "$$opensil_path" ] && [ -d "$$opensil_path" ]; then \
		comm_hash=$$(git -c safe.directory='*' -C "$$opensil_path" log -n 1 --format=%H 2>/dev/null); \
		if [ -n "$$comm_hash" ]; then \
			sed -i -e "s/<software_version>/$$comm_hash/" $@; \
		else \
			sed -i "/software-version/d" $@; \
		fi; \
	else \
		sed -i "/software-version/d" $@; \
	fi

$(build-dir)/intel-microcode-%.json: $(src-dir)/intel-microcode.json 3rdparty/intel-microcode/intel-ucode/% | $(build-dir) $(build-dir)/goswid
	cp $< $@
	year=$$(hexdump --skip 8 --length 2 --format '"%04x"' $(word 2,$^));\
	day=$$(hexdump --skip 10 --length 1 --format '"%02x"' $(word 2,$^));\
	month=$$(hexdump --skip 11 --length 1 --format '"%02x"' $(word 2,$^));\
	sed -i "s/<software_version>/$$year-$$month-$$day/" $@
	#TODO add cpuid (processor family, model, stepping) as extra attribute
	if [ -s "$(word 2,$^)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(word 2,$^))" \
			--hash "$$(sha256sum "$(word 2,$^)" | cut -d' ' -f1)"; \
	fi

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
define amd-ucode-sbom-rule
$(build-dir)/amd-microcode-$(basename $(notdir $(1))).json: $(src-dir)/amd-microcode.json $(1) | $(build-dir)
	cp $$< $$@
	year=$$$$(hexdump --skip 0 --length 2 --format '"%04x"' $(1)); \
	day=$$$$(hexdump --skip 2 --length 1 --format '"%02x"' $(1)); \
	month=$$$$(hexdump --skip 3 --length 1 --format '"%02x"' $(1)); \
	sed -i "s/<software_version>/$$$$year-$$$$month-$$$$day/" $$@
endef
# amd_microcode_bins is populated in src/soc/amd/common/block/cpu/Makefile.mk,
# which is included several rounds after src/sbom (breadth-first traversal).
# Deferring via postinclude-hooks ensures the rules are generated after all
# Makefile.mks have been processed and amd_microcode_bins is fully populated.
postinclude-hooks += $$(foreach ucode,$$(amd_microcode_bins),$$(eval $$(call amd-ucode-sbom-rule,$$(ucode))))

vboot-gitdir := $(shell git -C 3rdparty/vboot rev-parse --absolute-git-dir 2>/dev/null)

$(build-dir)/vboot.json: $(src-dir)/vboot.json $(if $(vboot-gitdir),$(vboot-gitdir)/HEAD,) | $(build-dir) $(build-dir)/goswid
	cp $< $@
	git_tree_hash=$$(git -C 3rdparty/vboot log -n 1 --format=%T); \
	git_comm_hash=$$(git -C 3rdparty/vboot log -n 1 --format=%H); \
	sed -i -e "s/<colloquial_version>/$$git_tree_hash/" -e "s/<software_version>/$$git_comm_hash/" $@
	# vboot is built from source as a per-stage static library rather than a
	# single blob; hash the ramstage vboot_fw.a as the representative artifact
	# so the component carries integrity info (CRA Annex I hash carry-through).
	if [ -s "$(VBOOT_LIB_ramstage)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(VBOOT_LIB_ramstage))" \
			--hash "$$(sha256sum "$(VBOOT_LIB_ramstage)" | cut -d' ' -f1)"; \
	fi

ipxe-gitdir := $(shell git -C payloads/external/iPXE/ipxe rev-parse --absolute-git-dir 2>/dev/null)

$(build-dir)/payload-iPXE.json: $(src-dir)/payload-iPXE.json $(if $(ipxe-gitdir),$(ipxe-gitdir)/HEAD,) | $(build-dir) $(build-dir)/goswid $(ipxe-swid-ready-dep)
	cp $< $@
	set -e; \
	git_tree_hash=$$(git --git-dir payloads/external/iPXE/ipxe/.git log -n 1 --format=%T); \
	git_comm_hash=$$(git --git-dir payloads/external/iPXE/ipxe/.git log -n 1 --format=%H); \
	sed -i -e "s/<colloquial_version>/$$git_tree_hash/" -e "s/<software_version>/$$git_comm_hash/" $@
	# Hash the built iPXE ROM image (the artifact linked into CBFS) so the
	# component carries integrity info (CRA Annex I hash carry-through).
	if [ -s "payloads/external/iPXE/ipxe/ipxe.rom" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "ipxe.rom" \
			--hash "$$(sha256sum "payloads/external/iPXE/ipxe/ipxe.rom" | cut -d' ' -f1)"; \
	fi

# edk2-platforms is a separate git repository compiled into the edk2 payload.
# Record its commit hash (source-version) and tree hash (colloquial-version),
# mirroring the iPXE/payload git-backed component rules.  The checkout lives at
# a deterministic path created during the edk2 payload build.
#
# edk2-platforms has no standalone binary artifact (it is linked into the edk2
# payload FD), so for a component-integrity hash we record a sha256 over its
# exact source tree via `git archive HEAD`.  The archive is deterministic for a
# given commit (git sets entry mtimes to the commit time and prepends the commit
# id), so this is a stable, reproducible digest of what was actually built.  It
# lets the component carry a CRA payload.file[].hash like the binary blobs do.
edk2-platforms-git-dir := payloads/external/edk2/workspace/edk2-platforms
edk2-platforms-gitdir := $(shell git -C $(edk2-platforms-git-dir) rev-parse --absolute-git-dir 2>/dev/null)

$(build-dir)/payload-edk2-platforms.json: $(src-dir)/payload-edk2-platforms.json $(if $(edk2-platforms-gitdir),$(edk2-platforms-gitdir)/HEAD,) | $(build-dir) $(build-dir)/goswid $(edk2-platforms-swid-ready-dep)
	cp $< $@
	set -e; \
	if [ -e "$(edk2-platforms-git-dir)/.git" ]; then \
		git_tree_hash=$$(git --git-dir $(edk2-platforms-git-dir)/.git log -n 1 --format=%T); \
		git_comm_hash=$$(git --git-dir $(edk2-platforms-git-dir)/.git log -n 1 --format=%H); \
		sed -i -e "s/<colloquial_version>/$$git_tree_hash/" -e "s/<software_version>/$$git_comm_hash/" $@; \
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
	# The GOP driver blob carries no embedded version; use the enclosing git
	# repo's HEAD commit hash as a proxy (tree hash as colloquial-version),
	# mirroring the Intel Flash Descriptor rule.
	set -e; \
	gop_git_root=$$(git -C "$$(dirname "$(CONFIG_EDK2_GOP_FILE)")" rev-parse --show-toplevel 2>/dev/null); \
	if [ -n "$$gop_git_root" ]; then \
		gop_comm_hash=$$(git -C "$$gop_git_root" log -n 1 --format=%H); \
		gop_tree_hash=$$(git -C "$$gop_git_root" log -n 1 --format=%T); \
		sed -i -e "s/<software_version>/$$gop_comm_hash/" \
		       -e "s/<colloquial_version>/$$gop_tree_hash/" $@; \
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

# Build payload SBOM metadata only after the payload is ready in regular builds.
# For standalone `make sbom`, use an existing checkout only.
$(payload-swid): $(payload-swid-template) | $(build-dir) $(build-dir)/goswid $(payload-swid-ready-dep)
	cp $< $@;\
	set -e; \
	git_tree_hash=$$(git --git-dir $(payload-git-dir-y)/.git log -n 1 --format=%T);\
	git_comm_hash=$$(git --git-dir $(payload-git-dir-y)/.git log -n 1 --format=%H);\
	sed -i -e "s/<colloquial_version>/$$git_tree_hash/" -e "s/<software_version>/$$git_comm_hash/" $@;
	if [ -s "$(CONFIG_PAYLOAD_FILE)" ]; then \
		$(build-dir)/goswid add-payload-file -o $@ -i $@ \
			--name "$(notdir $(CONFIG_PAYLOAD_FILE))" \
			--hash "$$(sha256sum "$(CONFIG_PAYLOAD_FILE)" | cut -d' ' -f1)"; \
	fi

## Standalone SBOM regeneration target
## Rebuilds build/sbom/sbom.uswid from existing build artifacts without
## requiring a full rebuild. Useful for updating the SBOM after post-build
## blob patching (e.g. btg_provision, create_eom swap ACMs).
## Usage: make sbom
## To re-inject into the ROM after regenerating: use cbfstool manually or
## via build.sh once that integration is added.
##
## ACM JSON files are deleted before rebuilding so they are always freshly
## extracted from the current ROM, reflecting any post-build ACM swap.
.PHONY: sbom sbom-acm-clean

sbom-acm-clean:
	$(if $(CONFIG_SBOM_BIOS_ACM_GENERATE),rm -f $(build-dir)/intel-bios-acm.json,)
	$(if $(CONFIG_SBOM_SINIT_ACM_GENERATE),rm -f $(build-dir)/intel-sinit-acm.json,)

## FIXME: this target doesn't reliably trigger regeneration of sbom.uswid, which
##        is easy to see by running `make --debug=b sbom` once or twice
sbom: $(build-dir)/sbom.uswid
