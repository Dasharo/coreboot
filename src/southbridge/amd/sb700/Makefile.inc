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
ramstage-y += reset.c
ramstage-y += sb700.c
ramstage-y += spi.c

endif
