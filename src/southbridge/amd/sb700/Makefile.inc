ifeq ($(CONFIG_SOUTHBRIDGE_AMD_SB700),y)

bootblock-y += bootblock.c

romstage-y += early_setup.c
romstage-y += ramtop.c
romstage-y += reset.c
romstage-y += smbus.c

ramstage-y += acpi.c
ramstage-y += reset.c
ramstage-y += sb700.c
ramstage-y += spi.c

endif
