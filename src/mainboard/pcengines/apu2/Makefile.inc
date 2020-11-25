# SPDX-License-Identifier: GPL-2.0-only

bootblock-y += pmutil.c
bootblock-y += bootblock.c
bootblock-y += bios_knobs.c

romstage-y += bios_knobs.c
romstage-y += BiosCallOuts.c
romstage-y += OemCustomize.c
romstage-y += gpio_ftns.c
romstage-y += s1_button.c
romstage-y += pmutil.c

postcar-y += bios_knobs.c

ramstage-y += bios_knobs.c
ramstage-y += BiosCallOuts.c
ramstage-y += OemCustomize.c
ramstage-y += gpio_ftns.c
ramstage-y += s1_button.c
ramstage-y += pmutil.c

# Order of names in SPD_SOURCES is important!
SPD_SOURCES  = HYNIX-2G-1333
SPD_SOURCES += HYNIX-4G-1333-ECC

subdirs-y += variants/$(VARIANT_DIR)
