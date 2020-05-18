# SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_AHCI_BIOS),y)
stripped_ahcibios_id = $(call strip_quotes,$(CONFIG_AHCI_BIOS_ID))
cbfs-files-$(CONFIG_AHCI_BIOS) += pci$(stripped_ahcibios_id).rom
pci$(stripped_ahcibios_id).rom-file := $(call strip_quotes,$(CONFIG_AHCI_BIOS_FILE))
pci$(stripped_ahcibios_id).rom-type := optionrom
endif

bootblock-y += bootblock.c
bootblock-y += bios_knobs.c

romstage-y += buildOpts.c
romstage-y += BiosCallOuts.c
romstage-y += OemCustomize.c
romstage-y += gpio_ftns.c
romstage-y += bios_knobs.c

postcar-y += bios_knobs.c

ramstage-y += bios_knobs.c
ramstage-y += buildOpts.c
ramstage-y += BiosCallOuts.c
ramstage-y += OemCustomize.c
ramstage-y += gpio_ftns.c

postcar-y += bios_knobs.c


# Order of names in SPD_SOURCES is important!
SPD_SOURCES = HYNIX-H5TQ2G83CFR
SPD_SOURCES += HYNIX-H5TQ4G83MFR
