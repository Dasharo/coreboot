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

OBJCOPY = $(GCC_PREFIX)objcopy

ifeq ($(CONFIG_EDK2_UEFIPAYLOAD),y)
BUILD_STR = -p UefiPayloadPkg/UefiPayloadPkg.dsc
endif
BUILD_STR += -t COREBOOT
BUILD_STR += -D BOOTLOADER=COREBOOT
ifneq ($(V),1)
BUILD_STR += -q
ifeq ($(CONFIG_EDK2_UEFIPAYLOAD),y)
BUILD_STR += -s
endif
endif

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
# CPU_TIMER_LIB_ENABLE		= TRUE
ifneq ($(CONFIG_EDK2_CPU_TIMER_LIB),y)
BUILD_STR += -D CPU_TIMER_LIB_ENABLE=FALSE
else
BUILD_STR += --pcd gUefiCpuPkgTokenSpaceGuid.PcdCpuCoreCrystalClockFrequency=$(CONFIG_CPU_XTAL_HZ)
endif
# SERIAL_TERMINAL	= TRUE
ifeq ($(CONFIG_EDK2_SERIAL_SUPPORT),y)
BUILD_STR += -D SERIAL_TERMINAL=TRUE
# PcdSerialRedirectionDefaultState	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_SERIAL_REDIRECTION_DEFAULT_ENABLE),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdSerialRedirectionDefaultState=TRUE
endif

ifeq ($(CONFIG_EDK2_HAVE_2ND_UART),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdHave2ndUart=TRUE
# PcdSerialRedirection2DefaultState	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_SERIAL_REDIRECTION2_DEFAULT_ENABLE),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdSerialRedirection2DefaultState=TRUE
endif
endif # CONFIG_EDK2_HAVE_2ND_UART

endif # SERIAL_TERMINAL
# FOLLOW_BGRT_SPEC		= FALSE
ifeq ($(CONFIG_EDK2_FOLLOW_BGRT_SPEC),y)
BUILD_STR += -D FOLLOW_BGRT_SPEC=TRUE
endif
# MAX_VARIABLE_SIZE            = 0x10000
ifeq ($(CONFIG_SMMSTORE_V2),y)
BUILD_STR += --pcd gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize=0x8000
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
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowPs2Option=TRUE
# SKIP_PS2_DETECT		= FALSE
ifeq ($(CONFIG_EDK2_SKIP_PS2_DETECT),y)
BUILD_STR += --pcd gUefiPayloadPkgTokenSpaceGuid.PcdSkipPs2Detect=TRUE
endif # SKIP_PS2_DETECT

else
BUILD_STR += --pcd gUefiPayloadPkgTokenSpaceGuid.PcdSkipPs2Detect=TRUE
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowPs2Option=FALSE
endif # PS2_KEYBOARD_ENABLE
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
# SYSTEM76_EC_LOGGING		= FALSE
ifeq ($(CONFIG_EDK2_SYSTEM76_EC_LOGGING),y)
BUILD_STR += -D SYSTEM76_EC_LOGGING=TRUE
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
# SETUP_PASSWORD_ENABLE		= FALSE
ifeq ($(CONFIG_EDK2_SETUP_PASSWORD),y)
BUILD_STR += -D SETUP_PASSWORD_ENABLE=TRUE
endif
# TPM_ENABLE			= TRUE
ifeq ($(CONFIG_EDK2_TPM_ENABLE),y)
BUILD_STR += -D TPM_ENABLE=TRUE
endif
# NETWORK_LAN_ROM		= FALSE
ifneq ($(CONFIG_EDK2_LAN_ROM_DRIVER),)
BUILD_STR += -D NETWORK_LAN_ROM=TRUE
endif
# DISABLE_MTRR_PROGRAMMING	= TRUE
ifneq ($(CONFIG_EDK2_DISABLE_MTRR_PROGRAMMING),y)
BUILD_STR += -D DISABLE_MTRR_PROGRAMMING=FALSE
endif
# VARIABLE_SUPPORT undefined
ifeq ($(CONFIG_CONFIG_SMMSTORE_V2),y)
BUILD_STR += -D VARIABLE_SUPPORT=SMMSTORE
endif
# IOMMU_ENABLE			= FALSE
ifeq ($(CONFIG_IOMMU_ENABLE),y)
BUILD_STR += -D IOMMU_ENABLE=TRUE
endif
# DASHARO_SYSTEM_FEATURES	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_SYSTEM_FEATURES),y)
BUILD_STR += -D DASHARO_SYSTEM_FEATURES_ENABLE=TRUE
endif
# DASHARO_SECURITY_OPTIONS	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_SECURITY_OPTIONS),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowSecurityMenu=TRUE
endif
# DASHARO_INTEL_ME_OPTIONS	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_INTEL_ME_OPTIONS),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowIntelMeMenu=TRUE
endif
# DASHARO_NETWORK_CONFIG	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_NETWORK_CONFIG),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowNetworkMenu=TRUE
endif
# DASHARO_USB_CONFIG		= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_USB_CONFIG),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowUsbMenu=TRUE
endif
# DASHARO_CHIPSET_CONFIG	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_CHIPSET_CONFIG),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowChipsetMenu=TRUE
endif
# Always set the default power failure state to some value
ifeq ($(CONFIG_HAVE_POWER_STATE_AFTER_FAILURE),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdDefaultPowerFailureState=$(CONFIG_MAINBOARD_POWER_FAILURE_STATE)
else
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdDefaultPowerFailureState=0
endif
# DASHARO_POWER_CONFIG		= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_POWER_CONFIG),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowPowerMenu=TRUE

ifneq ($(CONFIG_EDK2_SLEEP_TYPE_OPTION),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdPowerMenuShowSleepType=FALSE
endif # CONFIG_EDK2_SLEEP_TYPE_OPTION

ifeq ($(CONFIG_EDK2_FAN_CURVE_OPTION),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdPowerMenuShowFanCurve=TRUE
endif # CONFIG_EDK2_FAN_CURVE_OPTION

ifeq ($(CONFIG_EDK2_BATTERY_CONFIG_OPTION),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdPowerMenuShowBatteryThresholds=TRUE
endif

endif # CONFIG_EDK2_DASHARO_POWER_CONFIG
# DASHARO_PCI_CONFIG		= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_PCI_CONFIG),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowPciMenu=TRUE
endif
# DASHARO_MEMORY_CONFIG	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_MEMORY_CONFIG),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowMemoryMenu=TRUE
endif
# DASHARO_NETWORK_BOOT_DEFAULT_ENABLE	= FALSE
ifeq ($(CONFIG_EDK2_DASHARO_NETWORK_BOOT_DEFAULT_ENABLE),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdDefaultNetworkBootEnable=TRUE
endif
ifeq ($(CONFIG_EDK2_DASHARO_OC_WDT_ENABLE),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowOcWdtOptions=TRUE
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdOcWdtTimeoutDefault=$(CONFIG_EDK2_DASHARO_OC_WDT_TIMEOUT)
endif
# PCIEXP_SUPPORT_RESIZABLE_BARS	= FALSE
ifeq ($(CONFIG_PCIEXP_SUPPORT_RESIZABLE_BARS),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdPciMenuShowResizeableBars=TRUE
endif
# GOP_DRIVER			= FALSE
ifeq ($(CONFIG_EDK2_GOP_DRIVER),y)
BUILD_STR += -D USE_PLATFORM_GOP=TRUE
endif
# BATTERY_CHECK			= FALSE
ifeq ($(CONFIG_EDK2_ENABLE_BATTERY_CHECK),y)
BUILD_STR += -D BATTERY_CHECK=TRUE
endif
# LOAD_OPTION_ROMS		= TRUE
ifeq ($(CONFIG_EDK2_DISABLE_OPTION_ROMS),y)
BUILD_STR += -D LOAD_OPTION_ROMS=FALSE
endif
# PRINT_SOL_STRINGS		= FALSE
ifeq ($(CONFIG_EDK2_PRINT_SOL_STRINGS),y)
BUILD_STR += --pcd gUefiPayloadPkgTokenSpaceGuid.PcdPrintSolStrings=TRUE
endif
# CREATE_PREINSTALLED_BOOT_OPTIONS = FALSE
ifeq ($(CONFIG_EDK2_CREATE_PREINSTALLED_BOOT_OPTIONS),y)
BUILD_STR += --pcd gEfiMdeModulePkgTokenSpaceGuid.PcdCreatePreInstalledBootOptions=TRUE
endif
# SHOW_CAMERA_OPTION = FALSE
ifeq ($(CONFIG_EDK2_SHOW_CAMERA_OPTION),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdSecurityShowCameraOption=TRUE
endif
# SHOW_WIFI_BT_OPTION = FALSE
ifeq ($(CONFIG_EDK2_SHOW_WIFI_BT_OPTION),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdSecurityShowWiFiBtOption=TRUE
endif
# PERFORMANCE_MEASUREMENT_ENABLE = FALSE
ifeq ($(CONFIG_EDK2_PERFORMANCE_MEASUREMENT_ENABLE),y)
BUILD_STR += -D PERFORMANCE_MEASUREMENT_ENABLE=TRUE
endif
ifneq ($(CONFIG_HAVE_INTEL_ME_HAP),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdIntelMeHapAvailable=FALSE
endif
ifneq ($(CONFIG_INTEL_ME_DEFAULT_STATE),)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdIntelMeDefaultState=$(CONFIG_INTEL_ME_DEFAULT_STATE)
endif
# RAM_DISK_ENABLE		= FALSE
ifeq ($(CONFIG_EDK2_RAM_DISK_ENABLE),y)
BUILD_STR += -D RAM_DISK_ENABLE=TRUE
endif
# APU_CONFIG_ENABLE		= FALSE
ifeq ($(CONFIG_VENDOR_PCENGINES),y)
BUILD_STR += -D APU_CONFIG_ENABLE=TRUE
endif

BUILD_STR += -D BOOT_MENU_KEY=$(CONFIG_EDK2_BOOT_MENU_KEY)
BUILD_STR += -D SETUP_MENU_KEY=$(CONFIG_EDK2_SETUP_MENU_KEY)

# PcdShowLockBios		= FALSE
ifeq ($(CONFIG_EDK2_ENABLE_BIOS_LOCK),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowLockBios=TRUE
endif
# PcdShowSmmBwp			= FALSE
ifeq ($(CONFIG_EDK2_ENABLE_SMM_BWP),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowSmmBwp=TRUE
endif
# PcdSleepTypeDefaultS3		= FALSE
ifeq ($(CONFIG_EDK2_S3_SLEEP_DEFAULT),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdSleepTypeDefaultS3=TRUE
endif
# PcdShowFum			= TRUE
ifneq ($(CONFIG_EDK2_ENABLE_FUM),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowFum=FALSE
endif
# UART_ON_SUPERIO		= FALSE
ifeq ($(CONFIG_EDK2_SERIAL_ON_SUPERIO),y)
BUILD_STR += -D UART_ON_SUPERIO=TRUE
endif
# PcdShowCpuMenu = FALSE
ifeq ($(CONFIG_EDK2_DASHARO_CPU_CONFIG),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowCpuMenu=TRUE
endif
# PcdShowCpuCoreDisable = FALSE
ifeq ($(CONFIG_EDK2_CORE_DISABLE_OPTION),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowCpuCoreDisable=TRUE
endif
# PcdShowCpuHyperThreading = FALSE
ifeq ($(CONFIG_EDK2_HYPERTHREADING_OPTION),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdShowCpuHyperThreading=TRUE
endif
# PcdCpuHyperThreadingDefault = TRUE
ifneq ($(CONFIG_EDK2_HYPERTHREADING_DEFAULT_STATE),y)
BUILD_STR += --pcd gDasharoSystemFeaturesTokenSpaceGuid.PcdCpuHyperThreadingDefault=FALSE
endif

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

ifeq ($(CONFIG_TPM_GOOGLE_CR50)$(CONFIG_EDK2_REPO_MRCHROMEBOX),yy)
BUILD_STR += -D TPM_ENABLE=FALSE
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
		if git status --ignore-submodules=dirty | grep -q "nothing to commit, working tree clean"; then \
			echo "    Checking out edk2-platforms revision $(CONFIG_EDK2_PLATFORMS_TAG_OR_REV)"; \
			git checkout --detach $(CONFIG_EDK2_PLATFORMS_TAG_OR_REV) -f; \
		else \
			echo "    Working directory not clean; will not overwrite"; \
		fi; \
		git submodule update --init --checkout

$(EDK2_PATH): $(WORKSPACE)
	if [ ! -d "$(EDK2_PATH)" ]; then \
		git clone --recurse-submodules $(CONFIG_EDK2_REPOSITORY) $(EDK2_PATH) -j5; \
	fi
	cd $(EDK2_PATH); \
		git reset --hard HEAD > /dev/null 2>&1 || true; \
		git clean -df > /dev/null 2>&1 || true; \
		if [ -e UefiPayloadPkg/ShimLayer/UniversalPayload.o ]; then \
			rm UefiPayloadPkg/ShimLayer/UniversalPayload.o; \
		fi; \
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

gop_driver:
	if [ -n "$(CONFIG_EDK2_GOP_DRIVER)" ]; then \
		echo "Using GOP driver $(CONFIG_EDK2_GOP_FILE)"; \
		cp $(top)/$(CONFIG_EDK2_GOP_FILE) $(EDK2_PATH)/UefiPayloadPkg/IntelGopDriver.efi; \
		cp $(top)/$(CONFIG_INTEL_GMA_VBT_FILE) $(EDK2_PATH)/UefiPayloadPkg/vbt.bin; \
	fi; \

lan_rom: $(EDK2_PATH)
	case "$(CONFIG_EDK2_LAN_ROM_DRIVER)" in \
		"") ;; \
		/*) mkdir -p $(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/; \
		    cp $(CONFIG_EDK2_LAN_ROM_DRIVER) \
				$(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/LanRom.efi;; \
		*) mkdir -p $(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/; \
		   cp $(top)/$(CONFIG_EDK2_LAN_ROM_DRIVER) \
				$(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/LanRom.efi;; \
	esac \

ipxe_rom: $(EDK2_PATH)
	if [ "$(CONFIG_EDK2_IPXE)" == "y" ]; then \
		echo "    Including externally built iPXE"; \
		mkdir -p $(EDK2_PATH)/UefiPayloadPkg/NetworkDrivers/; \
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

prep: $(EDK2_PATH) $(EDK2_PLATFORMS_PATH) clean checktools logo gop_driver lan_rom ipxe_rom
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

$(WORKSPACE)/Build/UefiPayloadPkgX64/UniversalPayload.elf: \
	prep print
	cd $(WORKSPACE); \
		source $(EDK2_PATH)/edksetup.sh; \
		echo -n "EDK2: Building... "; \
		$(EDK2_PATH)/UefiPayloadPkg/UniversalPayloadBuild.sh -a IA32 -b $(RELEASE_STR) $(BUILD_STR)
		if [ ! -f $@ ]; then \
			echo "Failed!"; \
			exit 1; \
		fi
		echo "Success!"; \

$(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/IA32/UefiPayloadPkg/ShimLayer/ShimLayer/DEBUG/ShimLayer.dll: \
		$(WORKSPACE)/Build/UefiPayloadPkgX64/UniversalPayload.elf prep
	cd $(WORKSPACE)/Build/UefiPayloadPkgX64 && \
		$(OBJCOPY) -I binary UniversalPayload.elf -O elf32-i386 -B i386 \
			$(EDK2_PATH)/UefiPayloadPkg/ShimLayer/UniversalPayload.o; \
	cd $(WORKSPACE) && \
		source $(EDK2_PATH)/edksetup.sh; \
		build -p UefiPayloadPkg/UefiPayloadPkg.dsc -b $(RELEASE_STR) -a IA32 -a X64 \
			-m UefiPayloadPkg/ShimLayer/ShimLayer.inf \
			-t COREBOOT -D BOOTLOADER=COREBOOT -D SHIMLAYER=TRUE \
			-y $(WORKSPACE)/Build/UefiPayloadPkgX64/ShimLayer.txt

UefiPayloadPkg: $(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/FV/UEFIPAYLOAD.fd
	mv $(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/FV/UEFIPAYLOAD.fd \
		../../../build/UEFIPAYLOAD.fd

UniversalPayload: $(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/IA32/UefiPayloadPkg/ShimLayer/ShimLayer/DEBUG/ShimLayer.dll
	mv $(WORKSPACE)/Build/UefiPayloadPkgX64/$(RELEASE_STR)_COREBOOT/IA32/UefiPayloadPkg/ShimLayer/ShimLayer/DEBUG/ShimLayer.dll \
		../../../build/ShimmedUniversalPayload.elf

clean:
	test -d $(WORKSPACE) && (cd $(WORKSPACE); rm -rf Build; rm -f Conf/tools_def.txt) || exit 0

distclean:
	rm -rf $(WORKSPACE)

.PHONY: $(EDK2_PATH) checktools logo UefiPayloadPkg UniversalPayload clean distclean
