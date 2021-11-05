ifeq ($(CONFIG_SOUTHBRIDGE_AMD_SB700),y)

bootblock-y += bootblock.c
bootblock-y += reset.c

verstage-y += reset.c

romstage-y += early_setup.c
romstage-y += ramtop.c
romstage-y += reset.c
romstage-y += smbus.c

postcar-y += reset.c

ramstage-y += acpi.c
ramstage-y += hda.c
ramstage-y += ide.c
ramstage-y += lpc.c
ramstage-y += reset.c
ramstage-y += sata.c
ramstage-y += sb700.c
ramstage-y += sm.c
ramstage-y += smbus.c
ramstage-y += spi.c
ramstage-y += usb.c

endif
