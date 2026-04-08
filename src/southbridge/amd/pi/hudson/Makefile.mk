## SPDX-License-Identifier: BSD-3-Clause

# Copyright (c) 2012,        Advanced Micro Devices, Inc.
#               2013 - 2014, Sage Electronic Engineering, LLC

bootblock-y += bootblock.c
bootblock-y += early_setup.c
bootblock-$(CONFIG_USBDEBUG) += enable_usbdebug.c

romstage-y += early_setup.c
romstage-y += enable_usbdebug.c
romstage-$(CONFIG_HUDSON_IMC_FWM) += imc.c
romstage-y += smbus.c
romstage-y += smbus_spd.c

verstage-y += early_setup.c

postcar-y += lpc_simpledev.c

ramstage-y += enable_usbdebug.c
ramstage-$(CONFIG_HAVE_ACPI_TABLES) += fadt.c
ramstage-y += hda.c
ramstage-y += hudson.c
ramstage-y += ide.c
ramstage-$(CONFIG_HUDSON_IMC_FWM) += imc.c
ramstage-y += lpc.c
ramstage-y += pci.c
ramstage-y += pcie.c
ramstage-y += sata.c
ramstage-y += sd.c
ramstage-y += sm.c
ramstage-$(CONFIG_HAVE_SMI_HANDLER) += smi.c
ramstage-$(CONFIG_HAVE_SMI_HANDLER) += smi_util.c
ramstage-y += usb.c

all-y += reset.c

smm-y += smihandler.c
smm-y += smi_util.c
smm-y += lpc_simpledev.c

CPPFLAGS_common += -I$(src)/southbridge/amd/pi/hudson/include

# ROMSIG At ROMBASE + 0x20000:
# +-----------+---------------+----------------+------------+
# |0x55AA55AA |EC ROM Address |GEC ROM Address |USB3 ROM    |
# +-----------+---------------+----------------+------------+
# |PSPDIR ADDR|
# +-----------+
#
# EC ROM should be 64K aligned.

ifeq ($(CONFIG_AMD_FWM_POSITION_420000),y)
$(error "Invalid AMD FWM position: " $(CONFIG_AMD_FWM_POSITION_420000))
endif

HUDSON_FWM_POSITION=$(CONFIG_AMD_FWM_POSITION)

# CONFIG_AMD_FWM_POSITION only works for 16MB or bigger flashes.
# For smaller chips, the offset has to be adjusted to be relative.
ifeq ($(call int-lt, $(CONFIG_ROM_SIZE) 0x1000000),1)
ifeq ($(call int-gt, $(CONFIG_AMD_FWM_POSITION) $(CONFIG_ROM_SIZE)),1)
HUDSON_FWM_POSITION=$(call _tohex, $(call int-subtract, $(CONFIG_AMD_FWM_POSITION) $(CONFIG_ROM_SIZE)))
endif
endif

add_opt_prefix=$(if $(call strip_quotes, $(1)), $(2) $(call strip_quotes, $(1)), )

OPT_HUDSON_XHCI_FWM_FILE=$(call add_opt_prefix, $(CONFIG_HUDSON_XHCI_FWM_FILE), --xhci)
OPT_HUDSON_IMC_FWM_FILE=$(call add_opt_prefix, $(CONFIG_HUDSON_IMC_FWM_FILE), --imc)

$(obj)/amdfw.rom:	$(call strip_quotes, $(CONFIG_HUDSON_XHCI_FWM_FILE)) \
			$(call strip_quotes, $(CONFIG_HUDSON_IMC_FWM_FILE)) \
			$(call strip_quotes, $(CONFIG_AMD_PUBKEY_FILE)) \
			$(DEP_FILES) \
			$(AMDFWTOOL)
	rm -f $@
	@printf "    AMDFWTOOL  $(subst $(obj)/,,$(@))\n"
	$(AMDFWTOOL) \
		$(OPT_HUDSON_XHCI_FWM_FILE) \
		$(OPT_HUDSON_IMC_FWM_FILE) \
		--flashsize $(CONFIG_ROM_SIZE) \
		--location $(HUDSON_FWM_POSITION) \
		--config $(CONFIG_AMDFW_CONFIG_FILE) \
		--output	$@

cbfs-files-y += apu/amdfw
apu/amdfw-file := $(obj)/amdfw.rom
apu/amdfw-position := $(call int-add, \
	$(call int-subtract, 0xffffffff $(CONFIG_ROM_SIZE)) \
	1 \
	$(HUDSON_FWM_POSITION))
apu/amdfw-type := raw
