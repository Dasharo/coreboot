ifeq ($(CONFIG_SOUTHBRIDGE_AMD_SR5650),y)

bootblock-y += bootblock.c

ramstage-y += sr5650.c

endif
