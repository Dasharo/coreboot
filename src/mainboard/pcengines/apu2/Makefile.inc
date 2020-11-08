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

verstage_x86-y += bios_knobs.c
verstage_x86-y += pmutil.c

# Order of names in SPD_SOURCES is important!
SPD_SOURCES  = HYNIX-2G-1333
SPD_SOURCES += HYNIX-4G-1333-ECC

cbfs-files-y += bootorder
bootorder-file := bootorder_def
bootorder-type := raw

LINUX_OPTIONS=-I "initrd.cpio.xz"
LINUX_OPTIONS+=-C "console=ttyS0,115200 iomem=relaxed"

cbfs-files-y += linux/payload
linux/payload-file := bzImage
linux/payload-type := payload
linux/payload-compression := none
linux/payload-options := $(LINUX_OPTIONS)
linux/payload-COREBOOT-position := 0x100

subdirs-y += variants/$(VARIANT_DIR)
