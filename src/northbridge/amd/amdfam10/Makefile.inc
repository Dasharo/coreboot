ifeq ($(CONFIG_NORTHBRIDGE_AMD_AMDFAM10),y)

bootblock-y += bootblock.c

ramstage-y += northbridge.c

endif
