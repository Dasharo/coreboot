ifeq ($(CONFIG_DRIVERS_AMD_HYPERTRANSPORT),y)

romstage-y += h3finit.c ht_wrapper.c comlib.c AsPsNb.c h3ncmn.c

endif
