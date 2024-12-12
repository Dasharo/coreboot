## SPDX-License-Identifier: GPL-2.0-only

# set up payload config and version files for later inclusion
ifeq ($(CONFIG_PAYLOAD_BUILD_SEABIOS),y)
PAYLOAD_CONFIG=payloads/external/SeaBIOS/seabios/.config
PAYLOAD_VERSION=payloads/external/SeaBIOS/seabios/out/autoversion.h
endif
ifeq ($(CONFIG_PAYLOAD_FILO),y)
PAYLOAD_CONFIG=payloads/external/FILO/filo/.config
PAYLOAD_VERSION=payloads/external/FILO/filo/build/version.h
endif
ifeq ($(CONFIG_PAYLOAD_DEPTHCHARGE),y)
PAYLOAD_CONFIG=payloads/external/depthcharge/depthcharge/.config
$(PAYLOAD_CONFIG): payloads/external/depthcharge/depthcharge/build/depthcharge.elf
ifeq ($(call strip_quotes,$(CONFIG_MAINBOARD_DEPTHCHARGE)),)
  BOARD=$(call ws_to_under,$(call strip_quotes,$(call tolower,$(CONFIG_MAINBOARD_PART_NUMBER))))
else
  BOARD=$(CONFIG_MAINBOARD_DEPTHCHARGE)
endif
#TODO: Figure out version
endif

ifeq ($(CONFIG_PAYLOAD_LINUX)$(CONFIG_PAYLOAD_LINUXBOOT),y)
ifneq ($(strip $(call strip_quotes,$(CONFIG_LINUX_COMMAND_LINE))),)
      ADDITIONAL_PAYLOAD_CONFIG+=-C $(CONFIG_LINUX_COMMAND_LINE)
endif
ifneq ($(strip $(call strip_quotes,$(CONFIG_LINUX_INITRD)$(CONFIG_LINUXBOOT_INITRAMFS_PATH))),)
ifneq ($(CONFIG_LINUXBOOT_ARM64),y)
      ADDITIONAL_PAYLOAD_CONFIG+=-I $(CONFIG_LINUX_INITRD)$(CONFIG_LINUXBOOT_INITRAMFS_PATH)$(CONFIG_LINUXBOOT_INITRAMFS_SUFFIX)
      prebuilt-files += $(strip $(call strip_quotes,$(CONFIG_LINUX_INITRD)$(CONFIG_LINUXBOOT_INITRAMFS)))
endif
endif
endif
ifneq ($(strip $(call strip_quotes,$(CONFIG_PAYLOAD_OPTIONS))),)
      ADDITIONAL_PAYLOAD_CONFIG+=$(strip $(call strip_quotes,$(CONFIG_PAYLOAD_OPTIONS)))
endif

cbfs-files-y += $(CONFIG_CBFS_PREFIX)/payload
$(CONFIG_CBFS_PREFIX)/payload-file := $(CONFIG_PAYLOAD_FILE)
ifeq ($(CONFIG_PAYLOAD_IS_FLAT_BINARY),y)
$(CONFIG_CBFS_PREFIX)/payload-type := flat-binary
else
$(CONFIG_CBFS_PREFIX)/payload-type := payload
endif
$(CONFIG_CBFS_PREFIX)/payload-compression := $(CBFS_PAYLOAD_COMPRESS_FLAG)
$(CONFIG_CBFS_PREFIX)/payload-options := $(ADDITIONAL_PAYLOAD_CONFIG)

cbfs-files-$(CONFIG_INCLUDE_CONFIG_FILE) += payload_config
payload_config-file := $(PAYLOAD_CONFIG)
payload_config-type := raw

cbfs-files-$(CONFIG_INCLUDE_CONFIG_FILE) += payload_revision
payload_revision-file := $(PAYLOAD_VERSION)
payload_revision-type := raw

cbfs-files-$(CONFIG_GRUB2_INCLUDE_RUNTIME_CONFIG_FILE) += etc/grub.cfg
etc/grub.cfg-file := $(CONFIG_GRUB2_RUNTIME_CONFIG_FILE)
etc/grub.cfg-type := raw
etc/grub.cfg-required := the GRUB runtime configuration file ($(CONFIG_GRUB2_RUNTIME_CONFIG_FILE))

# SeaBIOS

SEABIOS_CC_OFFSET=$(if $(filter %ccache,$(HOSTCC)),2,1)
SEABIOS_TARGET_PATH=payloads/external/SeaBIOS/seabios/out/bios.bin.elf
$(SEABIOS_TARGET_PATH): $(DOTCONFIG)
	$(MAKE) -C payloads/external/SeaBIOS \
			HOSTCC="$(HOSTCC)" \
			CC=$(word $(SEABIOS_CC_OFFSET),$(CC_x86_32)) \
			CFLAGS="$(patsubst $(word $(SEABIOS_CC_OFFSET),$(CC_x86_32))%,,$(wordlist $(SEABIOS_CC_OFFSET),9999,$(CC_x86_32)))" \
			LD=$(word 1,$(LD_x86_32)) LDFLAGS="$(patsubst $(word 1,$(LD_x86_32))%,,$(LD_x86_32))" \
			OBJDUMP="$(OBJDUMP_x86_32)" \
			OBJCOPY="$(OBJCOPY_x86_32)" STRIP="$(STRIP_x86_32)" \
			AS="$(AS_x86_32)" IASL="$(IASL)" \
			CONFIG_SEABIOS_MASTER=$(CONFIG_SEABIOS_MASTER) \
			CONFIG_SEABIOS_STABLE=$(CONFIG_SEABIOS_STABLE) \
			CONFIG_SEABIOS_REVISION=$(CONFIG_SEABIOS_REVISION) \
			CONFIG_SEABIOS_REVISION_ID=$(CONFIG_SEABIOS_REVISION_ID) \
			CONFIG_PAYLOAD_CONFIGFILE=$(CONFIG_PAYLOAD_CONFIGFILE) \
			CONFIG_SEABIOS_THREAD_OPTIONROMS=$(CONFIG_SEABIOS_THREAD_OPTIONROMS) \
			CONFIG_SEABIOS_VGA_COREBOOT=$(CONFIG_SEABIOS_VGA_COREBOOT) \
			CONFIG_DRIVERS_UART_8250IO=$(CONFIG_DRIVERS_UART_8250IO) \
			CONFIG_DRIVERS_UART_8250MEM=$(CONFIG_DRIVERS_UART_8250MEM) \
			CONFIG_HUDSON_UART=$(CONFIG_HUDSON_UART) \
			CONFIG_CONSOLE_SERIAL=$(CONFIG_CONSOLE_SERIAL) \
			CONFIG_TTYS0_BASE=$(CONFIG_TTYS0_BASE) \
			CONFIG_SEABIOS_DEBUG_LEVEL=$(CONFIG_SEABIOS_DEBUG_LEVEL) \
			CONFIG_DRIVERS_UART_8250MEM_32=$(CONFIG_DRIVERS_UART_8250MEM_32) \
			CONFIG_ENABLE_HSUART=$(CONFIG_ENABLE_HSUART) \
			CONFIG_CONSOLE_UART_BASE_ADDRESS=$(CONFIG_CONSOLE_UART_BASE_ADDRESS) \
			CONFIG_SEABIOS_HARDWARE_IRQ=$(CONFIG_SEABIOS_HARDWARE_IRQ)

payloads/external/SeaBIOS/seabios/out/vgabios.bin: $(SEABIOS_TARGET_PATH)
payloads/external/SeaBIOS/seabios/.config: $(SEABIOS_TARGET_PATH)
payloads/external/SeaBIOS/seabios/out/autoversion.h: $(SEABIOS_TARGET_PATH)

cbfs-files-$(CONFIG_SEABIOS_SECONDARY_PAYLOAD) += img/seabios
img/seabios-file := $(SEABIOS_TARGET_PATH)
img/seabios-type := payload
img/seabios-compression := $(CBFS_SECONDARY_PAYLOAD_COMPRESS_FLAG)

# add a SeaBIOS bootorder file
ifneq ($(strip $(CONFIG_SEABIOS_BOOTORDER_FILE)),)
cbfs-files-y += bootorder
bootorder-file := $(strip $(CONFIG_SEABIOS_BOOTORDER_FILE))
bootorder-type := raw
endif

ifneq ($(CONFIG_SEABIOS_PS2_TIMEOUT),)
ifneq ($(CONFIG_SEABIOS_PS2_TIMEOUT),0)
$(call add_intermediate, seabios_ps2_timeout, $(CBFSTOOL))
	@printf "    SeaBIOS    Wait up to $(CONFIG_SEABIOS_PS2_TIMEOUT) ms for PS/2 keyboard controller initialization\n"
	$(if $(CONFIG_UPDATE_IMAGE),-$(CBFSTOOL) $< remove -n etc/ps2-keyboard-spinup 2>/dev/null)
	$(CBFSTOOL) $< add-int -i $(CONFIG_SEABIOS_PS2_TIMEOUT) -n etc/ps2-keyboard-spinup
endif
endif

ifeq ($(CONFIG_SEABIOS_ADD_SERCON_PORT_FILE),y)
$(call add_intermediate, seabios_sercon, $(CBFSTOOL))
	@printf "    SeaBIOS    Add sercon-port file\n"
	$(if $(CONFIG_UPDATE_IMAGE),-$(CBFSTOOL) $< remove -n etc/sercon-port 2>/dev/null)
	$(CBFSTOOL) $< add-int -i $(CONFIG_SEABIOS_SERCON_PORT_ADDR) -n etc/sercon-port
endif

ifneq ($(CONFIG_SEABIOS_USB_SIGATT_TIME),)
ifneq ($(CONFIG_SEABIOS_USB_SIGATT_TIME),0)
$(call add_intermediate, seabios_usb_sigatt_time, $(CBFSTOOL))
	@printf "    SeaBIOS    Add sercon-port file\n"
	$(if $(CONFIG_UPDATE_IMAGE),-$(CBFSTOOL) $< remove -n etc/usb-time-sigatt 2>/dev/null)
	$(CBFSTOOL) $< add-int -i $(CONFIG_SEABIOS_USB_SIGATT_TIME) -n etc/usb-time-sigatt
endif
endif

ifeq ($(CONFIG_SEABIOS_THREAD_OPTIONROMS),y)
$(call add_intermediate, seabios_thread_optionroms, $(CBFSTOOL))
	@printf "    SeaBIOS    Thread optionroms\n"
	$(if $(CONFIG_UPDATE_IMAGE),-$(CBFSTOOL) $< remove -n etc/threads 2>/dev/null)
	$(CBFSTOOL) $< add-int -i 2 -n etc/threads
endif

ifeq ($(CONFIG_PAYLOAD_SEAGRUB),y)
ifneq ($(CONFIG_SEAGRUB_ALLOW_SEABIOS_BOOTMENU),y)
$(call add_intermediate, seabios_bootmenu, $(CBFSTOOL))
	@printf "    SeaBIOS    Disable boot menu\n"
	$(if $(CONFIG_UPDATE_IMAGE),-$(CBFSTOOL) $< remove -n etc/show-boot-menu 2>/dev/null)
	$(CBFSTOOL) $< add-int -i 0 -n etc/show-boot-menu
else
$(call add_intermediate, seabios_bootmenu, $(CBFSTOOL))
	$(if $(CONFIG_UPDATE_IMAGE),-$(CBFSTOOL) $< remove -n etc/show-boot-menu 2>/dev/null)
endif
endif

# Depthcharge

payloads/external/depthcharge/depthcharge/build/depthcharge.elf depthcharge: $(DOTCONFIG) $(CBFSTOOL)
	$(MAKE) -C payloads/external/depthcharge \
		BOARD=$(BOARD) \
		MFLAGS= MAKEFLAGS= \
		DEPTHCHARGE_MASTER=$(CONFIG_DEPTHCHARGE_MASTER) \
		DEPTHCHARGE_STABLE=$(CONFIG_DEPTHCHARGE_STABLE) \
		DEPTHCHARGE_REVISION=$(CONFIG_DEPTHCHARGE_REVISION) \
		DEPTHCHARGE_REVISION_ID=$(CONFIG_DEPTHCHARGE_REVISION_ID) \
		OVERRIDE_DEFCONFIG=$(CONFIG_LP_DEFCONFIG_OVERRIDE)

# edk2

ifeq ($(CONFIG_EDK2_ENABLE_IPXE),y)
IPXE_EFI := payloads/external/iPXE/ipxe/ipxe.rom
endif

$(obj)/UEFIPAYLOAD.fd: $(DOTCONFIG) $(IPXE_EFI)
	$(MAKE) -C payloads/external/edk2 DasharoPayloadPkg \
		HOSTCC="$(HOSTCC)" \
		CC="$(HOSTCC)" \
		CONFIG_EDK2_REPOSITORY=$(CONFIG_EDK2_REPOSITORY) \
		CONFIG_EDK2_TAG_OR_REV=$(CONFIG_EDK2_TAG_OR_REV) \
		CONFIG_EDK2_UEFIPAYLOAD=$(CONFIG_EDK2_UEFIPAYLOAD) \
		CONFIG_EDK2_UNIVERSAL_PAYLOAD=$(CONFIG_EDK2_UNIVERSAL_PAYLOAD) \
		CONFIG_EDK2_REPO_OFFICIAL=$(CONFIG_EDK2_REPO_OFFICIAL) \
		CONFIG_EDK2_REPO_MRCHROMEBOX=$(CONFIG_EDK2_REPO_MRCHROMEBOX) \
		CONFIG_EDK2_REPO_CUSTOM=$(CONFIG_EDK2_REPO_CUSTOM) \
		CONFIG_EDK2_CPU_TIMER_LIB=$(CONFIG_EDK2_CPU_TIMER_LIB) \
		CONFIG_EDK2_CUSTOM_BUILD_PARAMS=$(CONFIG_EDK2_CUSTOM_BUILD_PARAMS) \
		CONFIG_EDK2_USE_EDK2_PLATFORMS=$(CONFIG_EDK2_USE_EDK2_PLATFORMS) \
		CONFIG_EDK2_PLATFORMS_REPOSITORY=$(CONFIG_EDK2_PLATFORMS_REPOSITORY) \
		CONFIG_EDK2_PLATFORMS_TAG_OR_REV=$(CONFIG_EDK2_PLATFORMS_TAG_OR_REV) \
		CONFIG_EDK2_DEBUG=$(CONFIG_EDK2_DEBUG) \
		CONFIG_EDK2_RELEASE=$(CONFIG_EDK2_RELEASE) \
		CONFIG_EDK2_BOOTSPLASH_FILE=$(CONFIG_EDK2_BOOTSPLASH_FILE) \
		CONFIG_EDK2_BOOT_MANAGER_ESCAPE=$(CONFIG_EDK2_BOOT_MANAGER_ESCAPE) \
		CONFIG_EDK2_BOOT_TIMEOUT=$(CONFIG_EDK2_BOOT_TIMEOUT) \
		CONFIG_EDK2_CBMEM_LOGGING=$(CONFIG_EDK2_CBMEM_LOGGING) \
		CONFIG_EDK2_FOLLOW_BGRT_SPEC=$(CONFIG_EDK2_FOLLOW_BGRT_SPEC) \
		CONFIG_EDK2_FULL_SCREEN_SETUP=$(CONFIG_EDK2_FULL_SCREEN_SETUP) \
		CONFIG_EDK2_HAVE_EFI_SHELL=$(CONFIG_EDK2_HAVE_EFI_SHELL) \
		CONFIG_EDK2_PRIORITIZE_INTERNAL=$(CONFIG_EDK2_PRIORITIZE_INTERNAL) \
		CONFIG_EDK2_PS2_SUPPORT=$(CONFIG_EDK2_PS2_SUPPORT) \
		CONFIG_EDK2_SERIAL_SUPPORT=$(CONFIG_EDK2_SERIAL_SUPPORT) \
		CONFIG_EDK2_SD_MMC_TIMEOUT=$(CONFIG_EDK2_SD_MMC_TIMEOUT) \
		CONFIG_ECAM_MMCONF_BASE_ADDRESS=$(CONFIG_ECAM_MMCONF_BASE_ADDRESS) \
		CONFIG_ECAM_MMCONF_LENGTH=$(CONFIG_ECAM_MMCONF_LENGTH) \
		CONFIG_PCIEXP_SUPPORT_RESIZABLE_BARS=$(CONFIG_PCIEXP_SUPPORT_RESIZABLE_BARS) \
		CONFIG_CPU_XTAL_HZ=$(CONFIG_CPU_XTAL_HZ) \
		CONFIG_SMMSTORE_V2=$(CONFIG_SMMSTORE_V2) \
		CONFIG_EDK2_SECURE_BOOT_SUPPORT=$(CONFIG_EDK2_SECURE_BOOT_SUPPORT) \
		CONFIG_EDK2_GOP_DRIVER=$(CONFIG_EDK2_GOP_DRIVER) \
		CONFIG_EDK2_GOP_FILE=$(CONFIG_EDK2_GOP_FILE) \
		CONFIG_INTEL_GMA_VBT_FILE=$(CONFIG_INTEL_GMA_VBT_FILE) \
		CONFIG_EDK2_DISABLE_TPM=$(CONFIG_EDK2_DISABLE_TPM) \
		GCC_CC_x86_32=$(GCC_CC_x86_32) \
		GCC_CC_x86_64=$(GCC_CC_x86_64) \
		GCC_CC_arm=$(GCC_CC_arm) \
		GCC_CC_arm64=$(GCC_CC_arm64) \
		OBJCOPY_x86_32=$(OBJCOPY_x86_32) \
		OBJCOPY_x86_64=$(OBJCOPY_x86_64) \
		OBJCOPY_arm=$(OBJCOPY_arm) \
		OBJCOPY_arm64=$(OBJCOPY_arm64) \
		MFLAGS= MAKEFLAGS= \
		CONFIG_EDK2_ABOVE_4G_MEMORY=$(CONFIG_EDK2_ABOVE_4G_MEMORY) \
		CONFIG_EDK2_BATTERY_CONFIG_OPTION=$(CONFIG_EDK2_BATTERY_CONFIG_OPTION) \
		CONFIG_EDK2_BOOT_MENU_KEY=$(CONFIG_EDK2_BOOT_MENU_KEY) \
		CONFIG_EDK2_CREATE_PREINSTALLED_BOOT_OPTIONS=$(CONFIG_EDK2_CREATE_PREINSTALLED_BOOT_OPTIONS) \
		CONFIG_EDK2_DASHARO_CHIPSET_CONFIG=$(CONFIG_EDK2_DASHARO_CHIPSET_CONFIG) \
		CONFIG_EDK2_DASHARO_INTEL_ME_OPTIONS=$(CONFIG_EDK2_DASHARO_INTEL_ME_OPTIONS) \
		CONFIG_EDK2_DASHARO_MEMORY_CONFIG=$(CONFIG_EDK2_DASHARO_MEMORY_CONFIG) \
		CONFIG_EDK2_DASHARO_NETWORK_BOOT_DEFAULT_ENABLE=$(CONFIG_EDK2_DASHARO_NETWORK_BOOT_DEFAULT_ENABLE) \
		CONFIG_EDK2_DASHARO_NETWORK_CONFIG=$(CONFIG_EDK2_DASHARO_NETWORK_CONFIG) \
		CONFIG_EDK2_DASHARO_OC_WDT_ENABLE=$(CONFIG_SOC_INTEL_COMMON_OC_WDT_ENABLE) \
		CONFIG_EDK2_DASHARO_OC_WDT_TIMEOUT=$(CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT_SECONDS) \
		CONFIG_EDK2_DASHARO_PCI_CONFIG=$(CONFIG_EDK2_DASHARO_PCI_CONFIG) \
		CONFIG_EDK2_DASHARO_POWER_CONFIG=$(CONFIG_EDK2_DASHARO_POWER_CONFIG) \
		CONFIG_EDK2_DASHARO_SECURITY_OPTIONS=$(CONFIG_EDK2_DASHARO_SECURITY_OPTIONS) \
		CONFIG_EDK2_DASHARO_SERIAL_REDIRECTION2_DEFAULT_ENABLE=$(CONFIG_EDK2_DASHARO_SERIAL_REDIRECTION2_DEFAULT_ENABLE) \
		CONFIG_EDK2_DASHARO_SERIAL_REDIRECTION_DEFAULT_ENABLE=$(CONFIG_EDK2_DASHARO_SERIAL_REDIRECTION_DEFAULT_ENABLE) \
		CONFIG_EDK2_DASHARO_SYSTEM_FEATURES=$(CONFIG_EDK2_DASHARO_SYSTEM_FEATURES) \
		CONFIG_EDK2_DASHARO_USB_CONFIG=$(CONFIG_EDK2_DASHARO_USB_CONFIG) \
		CONFIG_EDK2_DISABLE_MTRR_PROGRAMMING=$(CONFIG_EDK2_DISABLE_MTRR_PROGRAMMING) \
		CONFIG_EDK2_DISABLE_OPTION_ROMS=$(CONFIG_EDK2_DISABLE_OPTION_ROMS) \
		CONFIG_EDK2_ENABLE_BATTERY_CHECK=$(CONFIG_EDK2_ENABLE_BATTERY_CHECK) \
		CONFIG_EDK2_ENABLE_BIOS_LOCK=$(CONFIG_BOOTMEDIA_LOCK_WPRO_VBOOT_RO) \
		CONFIG_EDK2_ENABLE_FUM=$(CONFIG_DASHARO_FIRMWARE_UPDATE_MODE) \
		CONFIG_EDK2_ENABLE_SMM_BWP=$(CONFIG_BOOTMEDIA_SMM_BWP) \
		CONFIG_EDK2_FAN_CURVE_OPTION=$(CONFIG_EDK2_FAN_CURVE_OPTION) \
		CONFIG_EDK2_FAN_OFF_CURVE_OPTION=$(CONFIG_EDK2_FAN_OFF_CURVE_OPTION) \
		CONFIG_EDK2_HAVE_2ND_UART=$(CONFIG_EDK2_HAVE_2ND_UART) \
		CONFIG_EDK2_IPXE=$(CONFIG_EDK2_ENABLE_IPXE) \
		CONFIG_EDK2_IPXE_OPTION_NAME=$(CONFIG_EDK2_IPXE_OPTION_NAME) \
		CONFIG_EDK2_LAN_ROM_DRIVER=$(CONFIG_EDK2_LAN_ROM_DRIVER) \
		CONFIG_EDK2_OPAL_PASSWORD=$(CONFIG_EDK2_OPAL_PASSWORD) \
		CONFIG_EDK2_PERFORMANCE_MEASUREMENT_ENABLE=$(CONFIG_EDK2_PERFORMANCE_MEASUREMENT_ENABLE) \
		CONFIG_EDK2_PRINT_SOL_STRINGS=$(CONFIG_EDK2_PRINT_SOL_STRINGS) \
		CONFIG_EDK2_RAM_DISK_ENABLE=$(CONFIG_EDK2_RAM_DISK_ENABLE) \
		CONFIG_EDK2_S3_SLEEP_DEFAULT=$(CONFIG_DASHARO_PREFER_S3_SLEEP) \
		CONFIG_EDK2_SATA_PASSWORD=$(CONFIG_EDK2_SATA_PASSWORD) \
		CONFIG_EDK2_SECURE_BOOT_DEFAULT_ENABLE=$(CONFIG_EDK2_SECURE_BOOT_DEFAULT_ENABLE) \
		CONFIG_EDK2_SERIAL_ON_SUPERIO=$(CONFIG_DRIVERS_UART_8250IO) \
		CONFIG_EDK2_SETUP_MENU_KEY=$(CONFIG_EDK2_SETUP_MENU_KEY) \
		CONFIG_EDK2_SETUP_PASSWORD=$(CONFIG_EDK2_SETUP_PASSWORD) \
		CONFIG_EDK2_SHOW_CAMERA_OPTION=$(CONFIG_EDK2_SHOW_CAMERA_OPTION) \
		CONFIG_EDK2_SHOW_WIFI_BT_OPTION=$(CONFIG_EDK2_SHOW_WIFI_BT_OPTION) \
		CONFIG_EDK2_SKIP_PS2_DETECT=$(CONFIG_EDK2_SKIP_PS2_DETECT) \
		CONFIG_EDK2_SLEEP_TYPE_OPTION=$(CONFIG_EDK2_SLEEP_TYPE_OPTION) \
		CONFIG_EDK2_SYSTEM76_EC_LOGGING=$(CONFIG_EDK2_SYSTEM76_EC_LOGGING) \
		CONFIG_EDK2_VERBOSE_BUILD=$(CONFIG_EDK2_VERBOSE_BUILD) \
		CONFIG_HAVE_INTEL_ME_HAP=$(CONFIG_HAVE_INTEL_ME_HAP) \
		CONFIG_HAVE_POWER_STATE_AFTER_FAILURE=$(CONFIG_HAVE_POWER_STATE_AFTER_FAILURE) \
		CONFIG_INTEL_ME_DEFAULT_STATE=$(CONFIG_INTEL_ME_DEFAULT_STATE) \
		CONFIG_IOMMU_ENABLE=$(CONFIG_ENABLE_EARLY_DMA_PROTECTION) \
		CONFIG_MAINBOARD_POWER_FAILURE_STATE=$(CONFIG_MAINBOARD_POWER_FAILURE_STATE) \
		CONFIG_VENDOR_PCENGINES=$(CONFIG_VENDOR_PCENGINES) \
		CONFIG_EDK2_DASHARO_OC_WDT_SHOW=$(CONFIG_EDK2_DASHARO_OC_WDT_SHOW) \
		CONFIG_EDK2_DASHARO_CPU_CONFIG=$(CONFIG_EDK2_DASHARO_CPU_CONFIG) \
		CONFIG_EDK2_CORE_DISABLE_OPTION=$(CONFIG_EDK2_CORE_DISABLE_OPTION) \
		CONFIG_EDK2_HYPERTHREADING_OPTION=$(CONFIG_EDK2_HYPERTHREADING_OPTION) \
		CONFIG_EDK2_HYPERTHREADING_DEFAULT_STATE=$(CONFIG_FSP_HYPERTHREADING) \
		CONFIG_EDK2_CPU_THROTTLING_THRESHOLD_OPTION=$(CONFIG_EDK2_CPU_THROTTLING_THRESHOLD_OPTION) \
		CONFIG_EDK2_CPU_THROTTLING_THRESHOLD_DEFAULT=$(CONFIG_EDK2_CPU_THROTTLING_THRESHOLD_DEFAULT) \
		CONFIG_CPU_MAX_TEMPERATURE=$(CONFIG_CPU_MAX_TEMPERATURE) \
		CONFIG_EDK2_USE_LAPTOP_LID_LIB=$(CONFIG_EDK2_USE_LAPTOP_LID_LIB) \
		CONFIG_EDK2_USE_UEFIVAR_BACKED_TPM_PPI=$(CONFIG_EDK2_USE_UEFIVAR_BACKED_TPM_PPI)


$(obj)/ShimmedUniversalPayload.elf: $(DOTCONFIG)
	$(MAKE) -C payloads/external/edk2 UniversalPayload \
		HOSTCC="$(HOSTCC)" \
		CC="$(HOSTCC)" \
		CONFIG_EDK2_REPOSITORY=$(CONFIG_EDK2_REPOSITORY) \
		CONFIG_EDK2_TAG_OR_REV=$(CONFIG_EDK2_TAG_OR_REV) \
		CONFIG_EDK2_UEFIPAYLOAD=$(CONFIG_EDK2_UEFIPAYLOAD) \
		CONFIG_EDK2_UNIVERSAL_PAYLOAD=$(CONFIG_EDK2_UNIVERSAL_PAYLOAD) \
		CONFIG_EDK2_REPO_OFFICIAL=$(CONFIG_EDK2_REPO_OFFICIAL) \
		CONFIG_EDK2_REPO_MRCHROMEBOX=$(CONFIG_EDK2_REPO_MRCHROMEBOX) \
		CONFIG_EDK2_REPO_CUSTOM=$(CONFIG_EDK2_REPO_CUSTOM) \
		CONFIG_EDK2_CPU_TIMER_LIB=$(CONFIG_EDK2_CPU_TIMER_LIB) \
		CONFIG_EDK2_CUSTOM_BUILD_PARAMS=$(CONFIG_EDK2_CUSTOM_BUILD_PARAMS) \
		CONFIG_EDK2_DEBUG=$(CONFIG_EDK2_DEBUG) \
		CONFIG_EDK2_RELEASE=$(CONFIG_EDK2_RELEASE) \
		CONFIG_EDK2_BOOTSPLASH_FILE=$(CONFIG_EDK2_BOOTSPLASH_FILE) \
		CONFIG_EDK2_BOOT_MANAGER_ESCAPE=$(CONFIG_EDK2_BOOT_MANAGER_ESCAPE) \
		CONFIG_EDK2_BOOT_TIMEOUT=$(CONFIG_EDK2_BOOT_TIMEOUT) \
		CONFIG_EDK2_CBMEM_LOGGING=$(CONFIG_EDK2_CBMEM_LOGGING) \
		CONFIG_EDK2_FOLLOW_BGRT_SPEC=$(CONFIG_EDK2_FOLLOW_BGRT_SPEC) \
		CONFIG_EDK2_FULL_SCREEN_SETUP=$(CONFIG_EDK2_FULL_SCREEN_SETUP) \
		CONFIG_EDK2_HAVE_EFI_SHELL=$(CONFIG_EDK2_HAVE_EFI_SHELL) \
		CONFIG_EDK2_PRIORITIZE_INTERNAL=$(CONFIG_EDK2_PRIORITIZE_INTERNAL) \
		CONFIG_EDK2_PS2_SUPPORT=$(CONFIG_EDK2_PS2_SUPPORT) \
		CONFIG_EDK2_SERIAL_SUPPORT=$(CONFIG_EDK2_SERIAL_SUPPORT) \
		CONFIG_EDK2_SD_MMC_TIMEOUT=$(CONFIG_EDK2_SD_MMC_TIMEOUT) \
		CONFIG_EDK2_UNIVERSAL_PAYLOAD=$(CONFIG_EDK2_UNIVERSAL_PAYLOAD) \
		CONFIG_ECAM_MMCONF_BASE_ADDRESS=$(CONFIG_ECAM_MMCONF_BASE_ADDRESS) \
		CONFIG_ECAM_MMCONF_LENGTH=$(CONFIG_ECAM_MMCONF_LENGTH) \
		CONFIG_CPU_XTAL_HZ=$(CONFIG_CPU_XTAL_HZ) \
		CONFIG_SMMSTORE_V2=$(CONFIG_SMMSTORE_V2) \
		GCC_CC_x86_32=$(GCC_CC_x86_32) \
		GCC_CC_x86_64=$(GCC_CC_x86_64) \
		GCC_CC_arm=$(GCC_CC_arm) \
		GCC_CC_arm64=$(GCC_CC_arm64) \
		OBJCOPY_x86_32=$(OBJCOPY_x86_32) \
		OBJCOPY_x86_64=$(OBJCOPY_x86_64) \
		OBJCOPY_arm=$(OBJCOPY_arm) \
		OBJCOPY_arm64=$(OBJCOPY_arm64) \
		MFLAGS= MAKEFLAGS=

# FILO

filo:
	$(MAKE) -C payloads/external/FILO \
			HOSTCC="$(HOSTCC)" \
			CC="$(CC_x86_32)" LD="$(LD_x86_32)" OBJDUMP="$(OBJDUMP_x86_32)" \
			OBJCOPY="$(OBJCOPY_x86_32)" STRIP="$(STRIP_x86_32)" \
			CONFIG_FILO_MASTER=$(CONFIG_FILO_MASTER) \
			CONFIG_FILO_STABLE=$(CONFIG_FILO_STABLE) \
			CONFIG_FILO_USE_AUTOBOOT=$(CONFIG_FILO_USE_AUTOBOOT) \
			CONFIG_FILO_AUTOBOOT_FILE=$(CONFIG_FILO_AUTOBOOT_FILE) \
			CONFIG_FILO_AUTOBOOT_DELAY=$(CONFIG_FILO_AUTOBOOT_DELAY)

payloads/external/FILO/filo/build/filo.elf: filo
payloads/external/FILO/filo/.config: filo
payloads/external/FILO/filo/build/version.h: filo

# Grub

GRUB_TARGET_PATH=payloads/external/GRUB2/grub2/build/default_payload.elf

cbfs-files-$(CONFIG_GRUB2_SECONDARY_PAYLOAD) += img/grub2
img/grub2-file := $(GRUB_TARGET_PATH)
img/grub2-type := payload
img/grub2-compression := $(CBFS_SECONDARY_PAYLOAD_COMPRESS_FLAG)

grub2: $(obj)/config.h
	$(MAKE) -C payloads/external/GRUB2 \
			HOSTCC="$(HOSTCC)" \
			CC="$(CC_x86_32)" LD="$(LD_x86_32)" \
			OBJCOPY="$(OBJCOPY_x86_32)" STRIP="$(STRIP_x86_32)" \
			CONFIG_DEP="$(abspath $(obj)/config.h)" \
			CONFIG_GRUB2_STABLE=$(CONFIG_GRUB2_STABLE) \
			CONFIG_GRUB2_MASTER=$(CONFIG_GRUB2_MASTER) \
			CONFIG_GRUB2_REVISION=$(CONFIG_GRUB2_REVISION) \
			CONFIG_GRUB2_REVISION_ID=$(CONFIG_GRUB2_REVISION_ID) \
			CONFIG_GRUB2_EXTRA_MODULES=$(CONFIG_GRUB2_EXTRA_MODULES)

$(GRUB_TARGET_PATH): grub2

# U-Boot

payloads/external/U-Boot/build/u-boot.bin u-boot: $(DOTCONFIG)
	$(MAKE) -C payloads/external/U-Boot \
			STABLE_COMMIT_ID=$(CONFIG_UBOOT_STABLE_COMMIT_ID) \
			CONFIG_UBOOT_MASTER=$(CONFIG_UBOOT_MASTER) \
			CONFIG_UBOOT_STABLE=$(CONFIG_UBOOT_STABLE)

# TINT

payloads/external/tint/tint/tint.elf tint:
	$(MAKE) -C payloads/external/tint

cbfs-files-$(CONFIG_TINT_SECONDARY_PAYLOAD) += img/tint
img/tint-file := payloads/external/tint/tint/tint.elf
img/tint-type := payload
img/tint-compression := $(CBFS_SECONDARY_PAYLOAD_COMPRESS_FLAG)

# Memtest86+

ifeq ($(CONFIG_MEMTEST86PLUS_V6),y)
memtest_dir:=memtest86plus_v6
else
memtest_dir:=memtest86plus_v5
endif

cbfs-files-$(CONFIG_MEMTEST_SECONDARY_PAYLOAD) += img/memtest
img/memtest-file := payloads/external/Memtest86Plus/$(memtest_dir)/memtest
img/memtest-type := payload
img/memtest-compression := $(CBFS_SECONDARY_PAYLOAD_COMPRESS_FLAG)

ifeq ($(CONFIG_CONSOLE_SERIAL)$(CONFIG_DRIVERS_UART_8250IO),yy)
	MEMTEST_SERIAL_OPTIONS=SERIAL_CONSOLE_DEFAULT=1 \
		SERIAL_TTY=$(CONFIG_UART_FOR_CONSOLE) \
		SERIAL_BAUD_RATE=$(CONFIG_TTYS0_BAUD)
endif

payloads/external/Memtest86Plus/$(memtest_dir)/memtest: $(DOTCONFIG)
	$(MAKE) -C payloads/external/Memtest86Plus all \
		CC="$(CC_x86_32)" \
		LD="$(LD_x86_32)" \
		OBJCOPY="$(OBJCOPY_x86_32)" \
		AS="$(AS_x86_32)" \
		CONFIG_MEMTEST_REVISION=$(CONFIG_MEMTEST_REVISION) \
		CONFIG_MEMTEST_REVISION_ID=$(CONFIG_MEMTEST_REVISION_ID) \
		CONFIG_MEMTEST_MAIN=$(CONFIG_MEMTEST_MAIN) \
		CONFIG_MEMTEST_STABLE=$(CONFIG_MEMTEST_STABLE) \
		CONFIG_MEMTEST86PLUS_V5=$(CONFIG_MEMTEST86PLUS_V5) \
		CONFIG_MEMTEST86PLUS_V6=$(CONFIG_MEMTEST86PLUS_V6) \
		$(MEMTEST_SERIAL_OPTIONS) \
		MFLAGS= MAKEFLAGS=

# iPXE

PXE_ROM_PCI_ID:=$(subst $(comma),,$(CONFIG_PXE_ROM_ID))

ifeq ($(CONFIG_PXE_ROM),y)
PXE_ROM_FILE:=$(CONFIG_PXE_ROM_FILE)
endif
ifeq ($(CONFIG_BUILD_IPXE),y)
PXE_ROM_FILE:=payloads/external/iPXE/ipxe/ipxe.rom
endif
ifeq ($(CONFIG_IPXE_ADD_SCRIPT),y)
IPXE_CONFIG_SCRIPT:=$(abspath $(patsubst "%",%,$(CONFIG_IPXE_SCRIPT)))
endif
ifeq ($(CONFIG_CONSOLE_SERIAL)$(CONFIG_DRIVERS_UART_8250IO),yy)
IPXE_UART=COM$(call int-add,$(CONFIG_UART_FOR_CONSOLE) 1)
endif

ifeq ($(CONFIG_IPXE_SERIAL_CONSOLE),y)
IPXE_SERIAL_CONSOLE = $(CONFIG_CONSOLE_SERIAL)$(CONFIG_DRIVERS_UART_8250IO)
else
IPXE_SERIAL_CONSOLE = n
endif

ifneq ($(CONFIG_EDK2_ENABLE_IPXE),y)
cbfs-files-$(CONFIG_PXE_ROM)$(CONFIG_BUILD_IPXE) += pci$(CONFIG_PXE_ROM_ID).rom
pci$(CONFIG_PXE_ROM_ID).rom-file := $(PXE_ROM_FILE)
pci$(CONFIG_PXE_ROM_ID).rom-type := raw
IPXE_CROSS_COMPILE:="$(CROSS_COMPILE_$(ARCH-ramstage-y))"
else
IPXE_CROSS_COMPILE:=$(CROSS_COMPILE_x86_64)
endif

payloads/external/iPXE/ipxe/ipxe.rom ipxe: $(DOTCONFIG) $(IPXE_CONFIG_SCRIPT)
	$(MAKE) -C payloads/external/iPXE all \
	CROSS_COMPILE=$(IPXE_CROSS_COMPILE) \
	PXE_ROM_PCI_ID=$(PXE_ROM_PCI_ID) \
	CONFIG_IPXE_MASTER=$(CONFIG_IPXE_MASTER) \
	CONFIG_IPXE_STABLE=$(CONFIG_IPXE_STABLE) \
	CONSOLE_SERIAL=$(IPXE_SERIAL_CONSOLE) \
	IPXE_UART=$(IPXE_UART) \
	CONFIG_TTYS0_BAUD=$(CONFIG_TTYS0_BAUD) \
	CONFIG_SCRIPT=$(IPXE_CONFIG_SCRIPT) \
	CONFIG_HAS_SCRIPT=$(CONFIG_IPXE_ADD_SCRIPT) \
	CONFIG_IPXE_NO_PROMPT=$(CONFIG_IPXE_NO_PROMPT) \
	CONFIG_IPXE_HAS_HTTPS=$(CONFIG_IPXE_HAS_HTTPS) \
	CONFIG_IPXE_CUSTOM_BUILD_ID=$(CONFIG_IPXE_CUSTOM_BUILD_ID) \
	MFLAGS= MAKEFLAGS= \
	CFLAGS_x86_64="$(CFLAGS_x86_64)" \
	CONFIG_IPXE_BUILD_EFI=$(CONFIG_EDK2_ENABLE_IPXE) \
	CONFIG_IPXE_TRUST_CMD=$(CONFIG_IPXE_TRUST_CMD)

# LinuxBoot
LINUXBOOT_CROSS_COMPILE_ARCH-$(CONFIG_LINUXBOOT_X86)        = x86_32
LINUXBOOT_CROSS_COMPILE_ARCH-$(CONFIG_LINUXBOOT_X86_64)     = x86_64
LINUXBOOT_CROSS_COMPILE_ARCH-$(CONFIG_LINUXBOOT_ARM)        = arm
LINUXBOOT_CROSS_COMPILE_ARCH-$(CONFIG_LINUXBOOT_ARM64)      = arm64
LINUXBOOT_CROSS_COMPILE_ARCH-$(CONFIG_LINUXBOOT_RISCV_RV32) = riscv
LINUXBOOT_CROSS_COMPILE_ARCH-$(CONFIG_LINUXBOOT_RISCV_RV64) = riscv
ifeq ($(CONFIG_LINUXBOOT_CROSS_COMPILE),"")
	CONFIG_LINUXBOOT_CROSS_COMPILE=$(CROSS_COMPILE_$(LINUXBOOT_CROSS_COMPILE_ARCH-y))
endif
.PHONY: linuxboot
payloads/external/LinuxBoot/build/Image linuxboot:
	$(MAKE) -C payloads/external/LinuxBoot \
		CPUS=$(CPUS) \
		CONFIG_LINUXBOOT_X86_64=$(CONFIG_LINUXBOOT_X86_64) \
		CONFIG_LINUXBOOT_X86=$(CONFIG_LINUXBOOT_X86) \
		CONFIG_LINUXBOOT_ARM=$(CONFIG_LINUXBOOT_ARM) \
		CONFIG_LINUXBOOT_ARM64=$(CONFIG_LINUXBOOT_ARM64) \
		CONFIG_LINUXBOOT_RISCV_RV32=$(CONFIG_LINUXBOOT_RISCV_RV32) \
		CONFIG_LINUXBOOT_RISCV_RV64=$(CONFIG_LINUXBOOT_RISCV_RV64) \
		CONFIG_LINUXBOOT_CROSS_COMPILE=$(CONFIG_LINUXBOOT_CROSS_COMPILE) \
		CONFIG_LINUXBOOT_BUILD_INITRAMFS=$(CONFIG_LINUXBOOT_BUILD_INITRAMFS) \
		CONFIG_LINUXBOOT_INITRAMFS_PATH=$(CONFIG_LINUXBOOT_INITRAMFS_PATH) \
		CONFIG_LINUXBOOT_INITRAMFS_SUFFIX=$(CONFIG_LINUXBOOT_INITRAMFS_SUFFIX) \
		CONFIG_LINUXBOOT_INITRAMFS_COMPRESSION_XZ=$(CONFIG_LINUXBOOT_INITRAMFS_COMPRESSION_XZ) \
		CONFIG_LINUXBOOT_COMPILE_KERNEL=$(CONFIG_LINUXBOOT_COMPILE_KERNEL) \
		CONFIG_LINUXBOOT_KERNEL_PATH=$(CONFIG_LINUXBOOT_KERNEL_PATH) \
		CONFIG_LINUXBOOT_KERNEL_VERSION=$(CONFIG_LINUXBOOT_KERNEL_VERSION) \
		CONFIG_LINUXBOOT_KERNEL_BZIMAGE=$(CONFIG_LINUXBOOT_KERNEL_BZIMAGE) \
		CONFIG_LINUXBOOT_KERNEL_UIMAGE=$(CONFIG_LINUXBOOT_KERNEL_UIMAGE) \
		CONFIG_LINUXBOOT_KERNEL_CONFIGFILE=$(CONFIG_LINUXBOOT_KERNEL_CONFIGFILE) \
		CONFIG_LINUXBOOT_UROOT=$(CONFIG_LINUXBOOT_UROOT) \
		CONFIG_LINUXBOOT_UROOT_VERSION=$(CONFIG_LINUXBOOT_UROOT_VERSION) \
		CONFIG_LINUXBOOT_UROOT_FORMAT=$(CONFIG_LINUXBOOT_UROOT_FORMAT) \
		CONFIG_LINUXBOOT_UROOT_INITCMD=$(CONFIG_LINUXBOOT_UROOT_INITCMD) \
		CONFIG_LINUXBOOT_UROOT_UINITCMD=$(CONFIG_LINUXBOOT_UROOT_UINITCMD) \
		CONFIG_LINUXBOOT_UROOT_SHELL=$(CONFIG_LINUXBOOT_UROOT_SHELL) \
		CONFIG_LINUXBOOT_UROOT_COMMANDS=$(CONFIG_LINUXBOOT_UROOT_COMMANDS) \
		CONFIG_LINUXBOOT_UROOT_FILES=$(CONFIG_LINUXBOOT_UROOT_FILES) \
		CONFIG_LINUXBOOT_DTS_FILE=$(CONFIG_LINUXBOOT_DTS_FILE)

# BOOTBOOT

payloads/external/BOOTBOOT/bootboot/dist/bootbootcb.elf:
	$(MAKE) -C payloads/external/BOOTBOOT all

# skiboot

payloads/external/skiboot/build/skiboot.elf:
	$(MAKE) -C payloads/external/skiboot all \
		CONFIG_SKIBOOT_GIT_REPO=$(CONFIG_SKIBOOT_GIT_REPO) \
		CONFIG_SKIBOOT_REVISION=$(CONFIG_SKIBOOT_REVISION)
# COREDOOM

payloads/external/coreDOOM/coredoom/doomgeneric/coredoom.elf coredoom:
	$(MAKE) -C payloads/external/coreDOOM

cbfs-files-$(CONFIG_COREDOOM_SECONDARY_PAYLOAD) += img/coreDOOM
img/coreDOOM-file := payloads/external/coreDOOM/coredoom/doomgeneric/coredoom.elf
img/coreDOOM-type := payload
img/coreDOOM-compression := $(CBFS_SECONDARY_PAYLOAD_COMPRESS_FLAG)
# WAD file
ifneq ($(strip $(CONFIG_COREDOOM_WAD_FILE)),)
cbfs-files-y += doom.wad
doom.wad-file := $(strip $(CONFIG_COREDOOM_WAD_FILE))
doom.wad-type := raw
doom.wad-compression := $(CBFS_SECONDARY_PAYLOAD_COMPRESS_FLAG)
endif
