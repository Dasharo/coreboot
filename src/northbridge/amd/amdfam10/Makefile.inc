ifeq ($(CONFIG_NORTHBRIDGE_AMD_AMDFAM10),y)

bootblock-y += bootblock.c

romstage-y += reset_test.c
romstage-y += romstage.c
romstage-y += raminit_sysinfo.c

ramstage-y += northbridge.c

endif
