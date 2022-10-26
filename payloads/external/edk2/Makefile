## SPDX-License-Identifier: GPL-2.0-only

# force the shell to bash - the edksetup.sh script doesn't work with dash
export SHELL := env bash

project_name = edk2
export WORKSPACE := $(CURDIR)/workspace
export EDK2_PATH := $(WORKSPACE)/$(word 3,$(subst /, ,$(CONFIG_EDK2_REPOSITORY)))

ifeq ($(CONFIG_EDK2_USE_EDK2_PLATFORMS),y)
export EDK2_PLATFORMS_PATH := $(WORKSPACE)/edk2-platforms
export PACKAGES_PATH := $(EDK2_PATH):\
			$(EDK2_PLATFORMS_PATH)/Platform/Intel:\
			$(EDK2_PLATFORMS_PATH)/Silicon/Intel:\
			$(EDK2_PLATFORMS_PATH)/Features/Intel:\
			$(EDK2_PLATFORMS_PATH)/Features/Intel/Debugging:\
			$(EDK2_PLATFORMS_PATH)/Features/Intel/Network:\
			$(EDK2_PLATFORMS_PATH)/Features/Intel/OutOfBandManagement:\
			$(EDK2_PLATFORMS_PATH)/Features/Intel/PowerManagement:\
			$(EDK2_PLATFORMS_PATH)/Features/Intel/SystemInformation:\
			$(EDK2_PLATFORMS_PATH)/Features/Intel/UserInterface
else
export PACKAGES_PATH := $(EDK2_PATH)
endif

BUILD_STR = -p UefiPayloadPkg/UefiPayloadPkg.dsc
BUILD_STR += -t COREBOOT
BUILD_STR += -D BOOTLOADER=COREBOOT
BUILD_STR += -q -s

#
# EDK II has the following build options relevant to coreboot:
#
#
# OPTION			= DEFAULT_VALUE
#
# ABOVE_4G_MEMORY		= TRUE
ifneq ($(CONFIG_EDK2_ABOVE_4G_MEMORY),y)
BUILD_STR += -D ABOVE_4G_MEMORY=FALSE
endif
# BOOTSPLASH_IMAGE		= FALSE
ifneq ($(CONFIG_EDK2_BOOTSPLASH_FILE),)
BUILD_STR += -D BOOTSPLASH_IMAGE=TRUE
endif
# BOOT_MANAGER_ESCAPE		= FALSE
ifeq ($(CONFIG_EDK2_BOOT_MANAGER_ESCAPE),y)
BUILD_STR += -D BOOT_MANAGER_ESCAPE=TRUE
endif
# BUILD_TARGETS			= DEBUG
ifeq ($(CONFIG_EDK2_DEBUG),y)
RELEASE_STR = DEBUG
else
RELEASE_STR = RELEASE
endif
# SERIAL_TERMINAL	= TRUE
ifeq ($(CONFIG_EDK2_SERIAL_SUPPORT),y)
BUILD_STR += -D SERIAL_TERMINAL=TRUE
endif
# FOLLOW_BGRT_SPEC		= FALSE
ifeq ($(CONFIG_EDK2_FOLLOW_BGRT_SPEC),y)
BUILD_STR += -D FOLLOW_BGRT_SPEC=TRUE
endif
# PCIE_BASE_ADDRESS            = 0
ifneq ($(CONFIG_ECAM_MMCONF_LENGTH),)
BUILD_STR += --pcd gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress=$(CONFIG_ECAM_MMCONF_BASE_ADDRESS)
endif
# PCIE_BASE_LENGTH             = 0
ifneq ($(CONFIG_ECAM_MMCONF_LENGTH),)
BUILD_STR += --pcd gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseSize=$(CONFIG_ECAM_MMCONF_LENGTH)
endif
# PRIORITIZE_INTERNAL		= FALSE
ifeq ($(CONFIG_EDK2_PRIORITIZE_INTERNAL),y)
BUILD_STR += -D PRIORITIZE_INTERNAL=TRUE
endif
# PS2_KEYBOARD_ENABLE		= FALSE
ifeq ($(CONFIG_EDK2_PS2_SUPPORT),y)
BUILD_STR += -D PS2_KEYBOARD_ENABLE=TRUE
endif
# PLATFORM_BOOT_TIMEOUT		= 3
ifneq ($(CONFIG_EDK2_BOOT_TIMEOUT),)
BUILD_STR += -D PLATFORM_BOOT_TIMEOUT=$(CONFIG_EDK2_BOOT_TIMEOUT)
endif
# SIO_BUS_ENABLE		= FALSE
ifeq ($(CONFIG_EDK2_PS2_SUPPORT),y)
BUILD_STR += -D SIO_BUS_ENABLE=TRUE
endif
# SHELL_TYPE			= BUILD_SHELL
ifneq ($(CONFIG_EDK2_HAVE_EFI_SHELL),y)
BUILD_STR += -D SHELL_TYPE=NONE
endif
# USE_CBMEM_FOR_CONSOLE		= FALSE
ifeq ($(CONFIG_EDK2_CBMEM_LOGGING),y)
BUILD_STR += -D USE_CBMEM_FOR_CONSOLE=TRUE
endif
# SD_MMC_TIMEOUT		= 1000000
ifneq ($(CONFIG_EDK2_SD_MMC_TIMEOUT),)
BUILD_STR += -D SD_MMC_TIMEOUT=$(shell echo $$(( $(CONFIG_EDK2_SD_MMC_TIMEOUT) * 1000)) )
endif
# NETWORK_IPXE                  = FALSE
ifeq ($(CONFIG_EDK2_IPXE),y)
BUILD_STR += -D NETWORK_IPXE=TRUE
BUILD_STR += --pcd gUefiPayloadPkgTokenSpaceGuid.PcdiPXEOptionName=L"$(CONFIG_EDK2_IPXE_OPTION_NAME)"
endif
# SECURE_BOOT_ENABLE            = TRUE
ifeq ($(CONFIG_EDK2_SECURE_BOOT),y)
BUILD_STR += -D SECURE_BOOT_ENABLE=TRUE
endif
# SECURE_BOOT_DEFAULT_ENABLE	= TRUE
ifneq ($(CONFIG_EDK2_SECURE_BOOT_DEFAULT_ENABLE),y)
BUILD_STR += -D SECURE_BOOT_DEFAULT_ENABLE=FALSE
endif
# SATA_PASSWORD_ENABLE		= TRUE
ifeq ($(CONFIG_EDK2_SATA_PASSWORD),y)
BUILD_STR += -D SATA_PASSWORD_ENABLE=TRUE
endif
# OPAL_PASSWORD_ENABLE		= TRUE
ifeq ($(CONFIG_EDK2_OPAL_PASSWORD),y)
BUILD_STR += -D OPAL_PASSWORD_ENABLE=TRUE
endif
# TPM_ENABLE		= TRUE
ifeq ($(CONFIG_EDK2_TPM_ENABLE),y)
BUILD_STR += -D TPM_ENABLE=TRUE
endif
ifneq ($(CONFIG_EDK2_LAN_ROM_DRIVER),)
BUILD_STR += -D NETWORK_LAN_ROM=TRUE
endif


BUILD_STR += -D BOOT_MENU_KEY=$(CONFIG_EDK2_BOOT_MENU_KEY)
BUILD_STR += -D SETUP_MENU_KEY=$(CONFIG_EDK2_SETUP_MENU_KEY)

#
# EDKII has the below PCDs that are relevant to coreboot:
#
# Allows EDKII to use the full framebuffer
ifeq ($(CONFIG_EDK2_FULL_SCREEN_SETUP),y)
BUILD_STR += --pcd gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow=0
BUILD_STR += --pcd gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn=0
BUILD_STR += --pcd gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutRow=0
BUILD_STR += --pcd gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutColumn=0
endif

bootloader = $(word 8,$(subst /, ,$(BUILD_STR)))

ifneq ($(CONFIG_EDK2_CUSTOM_BUILD_PARAMS),)
BUILD_STR += $(CONFIG_EDK2_CUSTOM_BUILD_PARAMS)
endif

all: UefiPayloadPkg

$(WORKSPACE):
	mkdir $(WORKSPACE)

$(EDK2_PLATFORMS_PATH): $(WORKSPACE)
	if [ ! -d "$(EDK2_PLATFORMS_PATH)" ]; then \
		git clone --recurse-submodules $(CONFIG_EDK2_PLATFORMS_REPOSITORY) $(EDK2_PLATFORMS_PATH) -j5; \
	fi
	cd $(EDK2_PLATFORMS_PATH); \
		if ! git rev-parse --verify -q $(CONFIG_EDK2_PLATFORMS_TAG_OR_REV) >/dev/null; then \
			echo "    $(CONFIG_EDK2_PLATFORMS_TAG_OR_REV) is not a valid git reference"; \
			exit 1; \
		fi; \
		if git status --ignore-submodules=dirty | grep -q clean; then \
			echo "    Checking out edk2-platforms revision $(CONFIG_EDK2_PLATFORMS_TAG_OR_REV)"; \
			git checkout --detach $(CONFIG_EDK2_PLATFORMS_TAG_OR_REV) -f; \
		else \
			echo "    Working directory not clean; will not overwrite"; \
		fi; \
		git submodule update --init --checkout

$(EDK2_PATH): $(WORKSPACE) $(EDK2_PLATFORMS_PATH)
	if [ ! -d "$(EDK2_PATH)" ]; then \
		git clone --recurse-submodules $(CONFIG_EDK2_REPOSITORY) $(EDK2_PATH) -j5; \
	fi
	cd $(EDK2_PATH); \
		git checkout MdeModulePkg/Logo/Logo.bmp > /dev/null 2>&1 || true; \
		echo "    Fetching new commits from $(CONFIG_EDK2_REPOSITORY)"; \
		git fetch origin 2>/dev/null; \
		if ! git rev-parse --verify -q $(CONFIG_EDK2_TAG_OR_REV) >/dev/null; then \
			echo "    $(CONFIG_EDK2_TAG_OR_REV) is not a valid git reference"; \
			exit 1; \
		fi; \
		if git status --ignore-submodules=dirty | grep -q clean; then \
			echo "    Checking out $(project_name) revision $(CONFIG_EDK2_TAG_OR_REV)"; \
			git checkout --detach $(CONFIG_EDK2_TAG_OR_REV) -f; \
		else \
			echo "    Working directory not clean; will not overwrite"; \
		fi; \
		git submodule update --init --checkout

logo: $(EDK2_PATH)
	case "$(CONFIG_EDK2_BOOTSPLASH_FILE)" in \
		"") ;; \
		/*) cp $(CONFIG_EDK2_BOOTSPLASH_FILE) \
				$(EDK2_PATH)/MdeModulePkg/Logo/Logo.bmp;; \
		*) cp $(top)/$(CONFIG_EDK2_BOOTSPLASH_FILE) \
				$(EDK2_PATH)/MdeModulePkg/Logo/Logo.bmp;; \
	esac \

lan_rom: $(EDK2_PATH)
	case "$(CONFIG_EDK2_LAN_ROM_DRIVER)" in \
		"") ;; \
		/*) cp $(CONFIG_EDK2_LAN_ROM_DRIVER) \
				$(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/LanRom.efi;; \
		*) cp $(top)/$(CONFIG_EDK2_LAN_ROM_DRIVER) \
				$(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/LanRom.efi;; \
	esac \

ipxe_rom: $(EDK2_PATH)
	if [ "$(CONFIG_EDK2_IPXE)" == "y" ]; then \
		echo "    Including externally built iPXE"; \
		cp $(top)/payloads/external/iPXE/ipxe/ipxe.rom \
			$(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/ipxe.efi; \
	fi \

checktools:
	echo -n "EDK2: Checking uuid-dev:"
	echo "#include <uuid/uuid.h>" > libtest.c
	echo "int main(int argc, char **argv) { (void) argc; (void) argv; return 0; }" >> libtest.c
	$(HOSTCC) $(HOSTCCFLAGS) libtest.c -o libtest >/dev/null 2>&1 && echo "    Found!" || \
		( echo "    Not found!"; \
			echo "ERROR: please_install uuid-dev (libuuid-devel)"; exit 1 )
	rm -rf libtest.c libtest
	echo -n "EDK2: Checking nasm:"
	type nasm > /dev/null 2>&1 && echo "        Found!" || \
		( echo "        Not found!"; echo "ERROR: Please install nasm."; exit 1 )

print:
	echo "   ##### $(project_name) Build Summary #####"
	echo "   Repository:     $(CONFIG_EDK2_REPOSITORY)"
	echo "   Branch:         $(CONFIG_EDK2_TAG_OR_REV)"
	echo "   Packages path:  $(PACKAGES_PATH)"
	echo " $(BUILD_STR)" | \
		sed -e 's/--/-/g' -e 's/-/\n   /g' | sort | sed \
		-e 's/a /Architecture:   /g' \
		-e 's/b /Release:        /g' \
		-e 's/D /Option:         /g' \
		-e 's/pcd /Pcd:            /g' \
		-e 's/p /Payload:        /g' \
		-e 's/q /Build:          Quiet/' \
		-e 's/s /Build:          Silent/' \
		-e 's/t /Toolchain:      /'

prep: $(EDK2_PATH) clean checktools logo lan_rom ipxe_rom
	cd $(WORKSPACE); \
		source $(EDK2_PATH)/edksetup.sh; \
		unset CC; $(MAKE) -C $(EDK2_PATH)/BaseTools 2>&1; \
		grep -q "COREBOOT" $(EDK2_PATH)/Conf/tools_def.txt; \
		if [ $$? -ne 0 ]; then \
			cat ../tools_def.txt >> $(EDK2_PATH)/Conf/tools_def.txt; \
		fi; \

$(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/FV/UEFIPAYLOAD.fd: \
	prep print
	cd $(WORKSPACE); \
		source $(EDK2_PATH)/edksetup.sh; \
		echo -n "EDK2: Building... "; \
		build -a IA32 -a X64 -b $(RELEASE_STR) $(BUILD_STR) \
			-y $(WORKSPACE)/Build/UefiPayloadPkgX64/UEFIPAYLOAD.txt; \
		if [ ! -f $@ ]; then \
			echo "Failed!"; \
			exit 1; \
		fi
		echo "Success!"; \

UefiPayloadPkg: $(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/FV/UEFIPAYLOAD.fd
	mv $(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/FV/UEFIPAYLOAD.fd \
		../../../build/UEFIPAYLOAD.fd

clean:
	test -d $(WORKSPACE) && (cd $(WORKSPACE); rm -rf Build; rm -f Conf/tools_def.txt) || exit 0

distclean:
	rm -rf $(WORKSPACE)

.PHONY: $(EDK2_PATH) checktools logo UefiPayloadPkg clean distclean
