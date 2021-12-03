ifeq ($(CONFIG_NORTHBRIDGE_AMD_AMDFAM10),y)

bootblock-y += bootblock.c

romstage-y += debug.c
romstage-y += pci.c
romstage-y += reset_test.c
romstage-y += romstage.c
romstage-y += raminit_amdmct.c
romstage-y += raminit_sysinfo.c
romstage-y += setup_resource_map.c

ramstage-y += acpi.c
ramstage-y += northbridge.c
ramstage-y += misc_control.c
ramstage-y += link_control.c
ramstage-y += nb_control.c

endif
